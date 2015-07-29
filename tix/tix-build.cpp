/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2014, 2015.

    This file is part of Tix.

    Tix is free software: you can redistribute it and/or modify it under the
    terms of the GNU General Public License as published by the Free Software
    Foundation, either version 3 of the License, or (at your option) any later
    version.

    Tix is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
    details.

    You should have received a copy of the GNU General Public License along with
    Tix. If not, see <https://www.gnu.org/licenses/>.

    tix-build.cpp
    Compile a source tix into a tix suitable for installation.

*******************************************************************************/

#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "util.h"

enum build_step
{
	BUILD_STEP_NO_SUCH_STEP,
	BUILD_STEP_START,
	BUILD_STEP_PRE_CLEAN,
	BUILD_STEP_CONFIGURE,
	BUILD_STEP_BUILD,
	BUILD_STEP_INSTALL,
	BUILD_STEP_POST_INSTALL,
	BUILD_STEP_PACKAGE,
	BUILD_STEP_POST_CLEAN,
	BUILD_STEP_END,
};

bool should_do_build_step(enum build_step step,
                   enum build_step start,
                   enum build_step end)
{
	return start <= step && step <= end;
}

#define SHOULD_DO_BUILD_STEP(step, minfo) \
        should_do_build_step((step), (minfo)->start_step, (minfo)->end_step)

enum build_step step_of_step_name(const char* step_name)
{
	if ( !strcmp(step_name, "start") )
		return BUILD_STEP_START;
	if ( !strcmp(step_name, "pre-clean") )
		return BUILD_STEP_PRE_CLEAN;
	if ( !strcmp(step_name, "configure") )
		return BUILD_STEP_CONFIGURE;
	if ( !strcmp(step_name, "build") )
		return BUILD_STEP_BUILD;
	if ( !strcmp(step_name, "install") )
		return BUILD_STEP_INSTALL;
	if ( !strcmp(step_name, "post-install") )
		return BUILD_STEP_POST_INSTALL;
	if ( !strcmp(step_name, "post-clean") )
		return BUILD_STEP_POST_CLEAN;
	if ( !strcmp(step_name, "package") )
		return BUILD_STEP_PACKAGE;
	if ( !strcmp(step_name, "clean") )
		return BUILD_STEP_POST_CLEAN;
	if ( !strcmp(step_name, "end") )
		return BUILD_STEP_END;
	return BUILD_STEP_NO_SUCH_STEP;
}

typedef struct
{
	char* build;
	char* build_dir;
	char* destination;
	char* host;
	char* make;
	char* makeflags;
	char* package_dir;
	char* package_info_path;
	char* package_name;
	char* prefix;
	char* exec_prefix;
	char* sysroot;
	char* tar;
	char* target;
	char* tmp;
	string_array_t package_info;
	enum build_step start_step;
	enum build_step end_step;
} metainfo_t;

bool has_in_path(const char* program)
{
	pid_t child_pid = fork();
	if ( child_pid < 0 )
		error(1, errno, "fork: which %s", program);
	if ( child_pid )
	{
		int exitstatus;
		waitpid(child_pid, &exitstatus, 0);
		return WIFEXITED(exitstatus) && WEXITSTATUS(exitstatus) == 0;
	}
	close(0); open("/dev/null", O_RDONLY);
	close(1); open("/dev/null", O_WRONLY);
	close(2); open("/dev/null", O_WRONLY);
	char* argv[] =
	{
		(char*) "which",
		(char*) program,
		(char*) NULL,
	};
	execvp(argv[0], argv);
	_exit(1);
}

void emit_compiler_wrapper_invocation(FILE* wrapper,
                                      metainfo_t* minfo,
                                      const char* name)
{
	fprintf(wrapper, "%s", name);
	if ( minfo->sysroot )
		fprintf(wrapper, " --sysroot=\"$SYSROOT\"");
	fprintf(wrapper, " \"$@\"");
}

void emit_compiler_warning_wrapper(metainfo_t* minfo,
                                   const char* bindir,
                                   const char* name)
{
	if ( !minfo->sysroot )
		return;
	if ( !has_in_path(name) )
		return;
	const char* warnings_dir = getenv("TIX_WARNINGS_DIR");
	char* wrapper_path = print_string("%s/%s", bindir, name);
	FILE* wrapper = fopen(wrapper_path, "w");
	if ( !wrapper )
		error(1, errno, "`%s'", wrapper_path);
	// TODO: Find a portable shell way of doing this.
	fprintf(wrapper, "#!/bin/bash\n");
	fprint_shell_variable_assignment(wrapper, "PATH", getenv("PATH"));
	fprint_shell_variable_assignment(wrapper, "TIX_WARNINGS_DIR", warnings_dir);
	if ( minfo->sysroot )
		fprint_shell_variable_assignment(wrapper, "SYSROOT", minfo->sysroot);
	fprintf(wrapper, "warnfile=$(mktemp --tmpdir=\"$TIX_WARNINGS_DIR\")\n");
	fprintf(wrapper, "(");
	emit_compiler_wrapper_invocation(wrapper, minfo, name);
	fprintf(wrapper, ") 2> >(tee $warnfile >&2)\n");
	fprintf(wrapper, "exitstatus=$?\n");
	fprintf(wrapper, "if test -s \"$warnfile\"; then\n");
	fprintf(wrapper, "  if test $exitstatus = 0; then\n");
	fprintf(wrapper, "    (echo \"cd $(pwd) && ");
	emit_compiler_wrapper_invocation(wrapper, minfo, name);
	fprintf(wrapper, " && cat \"$warnfile\") > \"$warnfile.warn\"\n");
	fprintf(wrapper, "  else\n");
	fprintf(wrapper, "    (echo \"cd $(pwd) && ");
	emit_compiler_wrapper_invocation(wrapper, minfo, name);
	fprintf(wrapper, " && cat \"$warnfile\") > \"$warnfile.err\"\n");
	fprintf(wrapper, "  fi\n");
	fprintf(wrapper, "fi\n");
	fprintf(wrapper, "rm -f \"$warnfile\"\n");
	fprintf(wrapper, "exit $exitstatus\n");
	fflush(wrapper);
	fchmod_plus_x(fileno(wrapper));
	fclose(wrapper);

	free(wrapper_path);
}

void emit_compiler_warning_cross_wrapper(metainfo_t* minfo,
                                         const char* bindir,
                                         const char* name)
{
	char* cross_name = print_string("%s-%s", minfo->host, name);
	emit_compiler_warning_wrapper(minfo, bindir, cross_name);
	free(cross_name);
}

void emit_compiler_sysroot_wrapper(metainfo_t* minfo,
                                   const char* bindir,
                                   const char* name)
{
	if ( !has_in_path(name) )
		return;
	char* wrapper_path = print_string("%s/%s", bindir, name);
	FILE* wrapper = fopen(wrapper_path, "w");
	if ( !wrapper )
		error(1, errno, "`%s'", wrapper_path);
	fprint_shell_variable_assignment(wrapper, "PATH", getenv("PATH"));
	if ( minfo->sysroot )
		fprint_shell_variable_assignment(wrapper, "SYSROOT", minfo->sysroot);
	fprintf(wrapper, "exec ");
	emit_compiler_wrapper_invocation(wrapper, minfo, name);
	fprintf(wrapper, "\n");
	fflush(wrapper);
	fchmod_plus_x(fileno(wrapper));
	fclose(wrapper);

	free(wrapper_path);
}

void emit_compiler_sysroot_cross_wrapper(metainfo_t* minfo,
                                         const char* bindir,
                                         const char* name)
{
	char* cross_name = print_string("%s-%s", minfo->host, name);
	emit_compiler_sysroot_wrapper(minfo, bindir, cross_name);
	free(cross_name);
}

void emit_pkg_config_wrapper(metainfo_t* minfo)
{
	char* bindir = print_string("%s/tmppid.%ju.bin", minfo->tmp, (uintmax_t) getpid());
	if ( mkdir_p(bindir, 0777) != 0 )
		error(1, errno, "mkdir: `%s'", bindir);

	on_exit(cleanup_file_or_directory, strdup(bindir));

	// Create a pkg-config script for the build system.
	char* pkg_config_for_build_path = print_string("%s/build-pkg-config", bindir);
	FILE* pkg_config_for_build = fopen(pkg_config_for_build_path, "w");
	if ( !pkg_config_for_build )
		error(1, errno, "`%s'", pkg_config_for_build_path);
	fprintf(pkg_config_for_build, "#!/bin/sh\n");
	fprint_shell_variable_assignment(pkg_config_for_build, "PATH", getenv("PATH"));
	fprint_shell_variable_assignment(pkg_config_for_build, "PKG_CONFIG", getenv("PKG_CONFIG"));
	fprint_shell_variable_assignment(pkg_config_for_build, "PKG_CONFIG_PATH", getenv("PKG_CONFIG_PATH"));
	fprint_shell_variable_assignment(pkg_config_for_build, "PKG_CONFIG_SYSROOT_DIR", getenv("PKG_CONFIG_SYSROOT_DIR"));
	fprint_shell_variable_assignment(pkg_config_for_build, "PKG_CONFIG_FOR_BUILD", getenv("PKG_CONFIG_FOR_BUILD"));
	fprint_shell_variable_assignment(pkg_config_for_build, "PKG_CONFIG_LIBDIR", getenv("PKG_CONFIG_LIBDIR"));
	fprintf(pkg_config_for_build, "exec ${PKG_CONFIG:-pkg-config} \"$@\"\n");
	fflush(pkg_config_for_build);
	fchmod_plus_x(fileno(pkg_config_for_build));
	fclose(pkg_config_for_build);
	free(pkg_config_for_build_path);

	// Create a pkg-config script for the host system.
	char* pkg_config_path = print_string("%s/pkg-config", bindir);
	FILE* pkg_config = fopen(pkg_config_path, "w");
	if ( !pkg_config )
		error(1, errno, "`%s'", pkg_config_path);
	fprintf(pkg_config, "#!/bin/sh\n");
	fprint_shell_variable_assignment(pkg_config, "PATH", getenv("PATH"));
	fprint_shell_variable_assignment(pkg_config, "PKG_CONFIG", getenv("PKG_CONFIG"));
	fprintf(pkg_config, "exec ${PKG_CONFIG:-pkg-config} --static \"$@\"\n");
	fflush(pkg_config);
	fchmod_plus_x(fileno(pkg_config));
	fclose(pkg_config);
	free(pkg_config_path);

	// Point to the correct pkg-config configuration through the environment.
	char* var_pkg_config = print_string("%s/pkg-config", bindir);
	char* var_pkg_config_for_build = print_string("%s/build-pkg-config", bindir);
	char* var_pkg_config_libdir =
		print_string("%s%s/lib/pkgconfig",
		             minfo->sysroot, minfo->exec_prefix);
	char* var_pkg_config_path = print_string("%s", var_pkg_config_libdir);
	char* var_pkg_config_sysroot_dir = print_string("%s", minfo->sysroot);
	setenv("PKG_CONFIG", var_pkg_config, 1);
	setenv("PKG_CONFIG_FOR_BUILD", var_pkg_config_for_build, 1);
	setenv("PKG_CONFIG_LIBDIR", var_pkg_config_libdir, 1);
	setenv("PKG_CONFIG_PATH", var_pkg_config_path, 1);
	setenv("PKG_CONFIG_SYSROOT_DIR", var_pkg_config_sysroot_dir, 1);
	free(var_pkg_config);
	free(var_pkg_config_for_build);
	free(var_pkg_config_libdir);
	free(var_pkg_config_path);
	free(var_pkg_config_sysroot_dir);

	if ( getenv("TIX_WARNINGS_DIR") )
	{
		char* warnings_dir = print_string("%s/%s", getenv("TIX_WARNINGS_DIR"), minfo->package_name);
		if ( mkdir(warnings_dir, 0777) == 0 || errno == EEXIST )
		{
			setenv("TIX_WARNINGS_DIR", warnings_dir, 1);
		}
		else
		{
			error(1, errno, "mkdir: `%s': compiler warnings won't be saved", warnings_dir);
			unsetenv("TIX_WARNINGS_DIR");
		}
		free(warnings_dir);
	}

	emit_compiler_sysroot_cross_wrapper(minfo, bindir, "cc");
	emit_compiler_sysroot_cross_wrapper(minfo, bindir, "gcc");
	emit_compiler_sysroot_cross_wrapper(minfo, bindir, "c++");
	emit_compiler_sysroot_cross_wrapper(minfo, bindir, "g++");
	emit_compiler_sysroot_cross_wrapper(minfo, bindir, "ld");

	if ( getenv("TIX_WARNINGS_DIR") )
	{
		emit_compiler_warning_wrapper(minfo, bindir, "cc");
		emit_compiler_warning_wrapper(minfo, bindir, "gcc");
		emit_compiler_warning_wrapper(minfo, bindir, "c++");
		emit_compiler_warning_wrapper(minfo, bindir, "g++");

		emit_compiler_warning_cross_wrapper(minfo, bindir, "cc");
		emit_compiler_warning_cross_wrapper(minfo, bindir, "gcc");
		emit_compiler_warning_cross_wrapper(minfo, bindir, "c++");
		emit_compiler_warning_cross_wrapper(minfo, bindir, "g++");
	}

	char* new_path = print_string("%s:%s", bindir, getenv("PATH") ? getenv("PATH") : "");
	setenv("PATH", new_path, 1);
	free(new_path);

	free(bindir);
}

void SetNeedVariableBuildTool(metainfo_t* minfo,
                              const char* variable,
                              const char* value)
{
	string_array_t* pkg_info = &minfo->package_info;
	const char* needed_vars = dictionary_get(pkg_info, "pkg.make.needed-vars", "true");
	char* key = print_string("pkg.make.needed-vars.%s", variable);
	const char* needed_var = dictionary_get(pkg_info, key, needed_vars);
	free(key);
	if ( !parse_boolean(needed_var) )
		return;
	setenv(variable, value, 1);
}

void SetNeedVariableCrossTool(metainfo_t* minfo,
                              const char* variable,
                              const char* value)
{
	if ( strcmp(minfo->build, minfo->host) == 0 )
	{
		SetNeedVariableBuildTool(minfo, variable, value);
	}
	else
	{
		char* newvalue = print_string("%s-%s", minfo->host, value);
		SetNeedVariableBuildTool(minfo, variable, newvalue);
		free(newvalue);
	}
}

void SetNeededVariables(metainfo_t* minfo)
{
	SetNeedVariableBuildTool(minfo, "AR_FOR_BUILD", "ar");
	SetNeedVariableBuildTool(minfo, "AS_FOR_BUILD", "as");
	SetNeedVariableBuildTool(minfo, "CC_FOR_BUILD", "gcc");
	SetNeedVariableBuildTool(minfo, "CPP_FOR_BUILD", "gcc -E");
	SetNeedVariableBuildTool(minfo, "CXXFILT_FOR_BUILD", "c++filt");
	SetNeedVariableBuildTool(minfo, "CXX_FOR_BUILD", "g++");
	SetNeedVariableBuildTool(minfo, "LD_FOR_BUILD", "ld");
	SetNeedVariableBuildTool(minfo, "NM_FOR_BUILD", "nm");
	SetNeedVariableBuildTool(minfo, "OBJCOPY_FOR_BUILD", "objcopy");
	SetNeedVariableBuildTool(minfo, "OBJDUMP_FOR_BUILD", "objdump");
	SetNeedVariableBuildTool(minfo, "RANLIB_FOR_BUILD", "ranlib");
	SetNeedVariableBuildTool(minfo, "READELF_FOR_BUILD", "readelf");
	SetNeedVariableBuildTool(minfo, "STRIP_FOR_BUILD", "strip");

	SetNeedVariableCrossTool(minfo, "AR", "ar");
	SetNeedVariableCrossTool(minfo, "AS", "as");
	SetNeedVariableCrossTool(minfo, "CC", "gcc");
	SetNeedVariableCrossTool(minfo, "CPP", "gcc -E");
	SetNeedVariableCrossTool(minfo, "CXXFILT", "c++filt");
	SetNeedVariableCrossTool(minfo, "CXX", "g++");
	SetNeedVariableCrossTool(minfo, "LD", "ld");
	SetNeedVariableCrossTool(minfo, "NM", "nm");
	SetNeedVariableCrossTool(minfo, "OBJCOPY", "objcopy");
	SetNeedVariableCrossTool(minfo, "OBJDUMP", "objdump");
	SetNeedVariableCrossTool(minfo, "RANLIB", "ranlib");
	SetNeedVariableCrossTool(minfo, "READELF", "readelf");
	SetNeedVariableCrossTool(minfo, "STRIP", "strip");
}

void Configure(metainfo_t* minfo)
{
	if ( fork_and_wait_or_recovery() )
	{
		string_array_t* pkg_info = &minfo->package_info;
		const char* configure_raw =
			dictionary_get(pkg_info, "pkg.configure.cmd", "./configure");
		char* configure;
		if ( strcmp(minfo->build_dir, minfo->package_dir) == 0 )
			configure = strdup(configure_raw);
		else
			configure = print_string("%s/%s", minfo->package_dir, configure_raw);
		const char* conf_extra_args =
			dictionary_get(pkg_info, "pkg.configure.args", "");
		const char* conf_extra_vars =
			dictionary_get(pkg_info, "pkg.configure.vars", "");
		bool with_sysroot =
			parse_boolean(dictionary_get(pkg_info, "pkg.configure.with-sysroot",
			                                        "false"));
		// After releasing Sortix 1.0, remove this and hard-code the default to
		// false. This allows building Sortix 0.9 with its own ports using this
		// tix-build version.
		const char* with_sysroot_ld_bug_default = "false";
		if ( !strcmp(minfo->package_name, "binutils") ||
		     !strcmp(minfo->package_name, "gcc") )
			with_sysroot_ld_bug_default = "true";
		bool with_sysroot_ld_bug =
			parse_boolean(dictionary_get(pkg_info, "pkg.configure.with-sysroot-ld-bug",
			                                        with_sysroot_ld_bug_default ));
		bool with_build_sysroot =
			parse_boolean(dictionary_get(pkg_info, "pkg.configure.with-build-sysroot",
			                                        "false"));
		if ( chdir(minfo->build_dir) != 0 )
			error(1, errno, "chdir: `%s'", minfo->build_dir);
		SetNeededVariables(minfo);
		string_array_t env_vars = string_array_make();
		string_array_append_token_string(&env_vars, conf_extra_vars);
		for ( size_t i = 0; i < env_vars.length; i++ )
		{
			char* key = env_vars.strings[i];
			assert(key);
			char* assignment = strchr((char*) key, '=');
			if ( !assignment )
			{
				if ( !strncmp(key, "unset ", strlen("unset ")) )
					unsetenv(key + strlen("unset "));
				continue;
			}
			*assignment = '\0';
			char* value = assignment+1;
			setenv(key, value, 1);
		}
		const char* fixed_cmd_argv[] =
		{
			configure,
			print_string("--build=%s", minfo->build),
			print_string("--host=%s", minfo->host),
			print_string("--target=%s", minfo->target),
			print_string("--prefix=%s", minfo->prefix),
			print_string("--exec-prefix=%s", minfo->exec_prefix),
			NULL
		};
		string_array_t args = string_array_make();
		for ( size_t i = 0; fixed_cmd_argv[i]; i++ )
			string_array_append(&args, fixed_cmd_argv[i]);
		if ( minfo->sysroot && with_build_sysroot )
		{
			string_array_append(&args, print_string("--with-build-sysroot=%s",
			                                        minfo->sysroot));
			if ( minfo->sysroot && with_sysroot )
			{
				// TODO: Binutils has a bug where the empty string means that
				//       sysroot support is disabled and ld --sysroot won't work
				//       so set it to / here for compatibility.
				// TODO: GCC has a bug where it doesn't use the
				//       --with-build-sysroot value when --with-sysroot= when
				//       locating standard library headers.
				if ( with_sysroot_ld_bug )
					string_array_append(&args, "--with-sysroot=/");
				else
					string_array_append(&args, "--with-sysroot=");
			}
		}
		else if ( minfo->sysroot && with_sysroot )
		{
			string_array_append(&args, print_string("--with-sysroot=%s",
			                                        minfo->sysroot));
		}
		string_array_append_token_string(&args, conf_extra_args);
		string_array_append(&args, NULL);
		recovery_execvp(args.strings[0], (char* const*) args.strings);
		error(127, errno, "`%s'", args.strings[0]);
	}
}

void Make(metainfo_t* minfo, const char* make_target,
          const char* destdir = NULL, bool die_on_error = true,
          const char* subdir = NULL)
{
	if ( (!die_on_error && fork_and_wait_or_death(die_on_error)) ||
	     (die_on_error && fork_and_wait_or_recovery()) )
	{
		string_array_t* pkg_info = &minfo->package_info;
		char* make = strdup(minfo->make);
		const char* override_make = dictionary_get(pkg_info, "pkg.make.cmd");
		const char* make_extra_args = dictionary_get(pkg_info, "pkg.make.args", "");
		const char* make_extra_vars = dictionary_get(pkg_info, "pkg.make.vars", "");
		if ( override_make )
		{
			free(make);
			make = join_paths(minfo->package_dir, override_make);
		}
		SetNeededVariables(minfo);
		if ( chdir(minfo->build_dir) != 0 )
			error(1, errno, "chdir: `%s'", minfo->build_dir);
		if ( subdir && chdir(subdir) != 0 )
			error(1, errno, "chdir: `%s/%s'", minfo->build_dir, subdir);
		if ( destdir )
			setenv("DESTDIR", destdir, 1);
		setenv("BUILD", minfo->build, 1);
		setenv("HOST", minfo->host, 1);
		setenv("TARGET", minfo->target, 1);
		if ( minfo->prefix )
			setenv("PREFIX", minfo->prefix, 1);
		else
			unsetenv("PREFIX");
		if ( minfo->exec_prefix )
			setenv("EXEC_PREFIX", minfo->exec_prefix, 1);
		else
			unsetenv("EXEC_PREFIX");
		if ( minfo->makeflags )
			setenv("MAKEFLAGS", minfo->makeflags, 1);
		setenv("MAKE", minfo->make, 1);
		string_array_t env_vars = string_array_make();
		string_array_append_token_string(&env_vars, make_extra_vars);
		for ( size_t i = 0; i < env_vars.length; i++ )
		{
			char* key = env_vars.strings[i];
			assert(key);
			char* assignment = strchr((char*) key, '=');
			if ( !assignment )
			{
				if ( !strncmp(key, "unset ", strlen("unset ")) )
					unsetenv(key + strlen("unset "));
				continue;
			}
			*assignment = '\0';
			char* value = assignment+1;
			setenv(key, value, 1);
		}
		const char* fixed_cmd_argv[] =
		{
			make,
			NULL
		};
		string_array_t args = string_array_make();
		for ( size_t i = 0; fixed_cmd_argv[i]; i++ )
			string_array_append(&args, fixed_cmd_argv[i]);
		string_array_append_token_string(&args, make_target);
		string_array_append_token_string(&args, make_extra_args);
		if ( !die_on_error )
			string_array_append(&args, "-k");
		string_array_append(&args, NULL);
		if ( die_on_error )
			recovery_execvp(args.strings[0], (char* const*) args.strings);
		else
			execvp(args.strings[0], (char* const*) args.strings);
		error(127, errno, "`%s'", args.strings[0]);
	}
}

void BuildPackage(metainfo_t* minfo)
{
	// Detect which build system we are interfacing with.
	string_array_t* pinfo = &minfo->package_info;
	const char* build_system = dictionary_get(pinfo, "pkg.build-system");
	assert(build_system);

	// Determine whether need to do an out-of-directory build.
	const char* use_build_dir_var =
		dictionary_get(pinfo, "pkg.configure.use-build-directory", "false");
	bool use_build_dir = parse_boolean(use_build_dir_var);
	if ( use_build_dir )
	{
		minfo->build_dir = print_string("%s/tmppid.%ju", minfo->tmp,
		                                (uintmax_t) getpid());
		if ( mkdir_p(minfo->build_dir, 0777) != 0 )
			error(1, errno, "mkdir: `%s'", minfo->build_dir);
	}
	else
		minfo->build_dir = strdup(minfo->package_dir);

	// Reset the build directory if needed.
	const char* default_clean_target =
		!strcmp(build_system, "configure") ? "distclean" : "clean";
	const char* clean_target = dictionary_get(pinfo, "pkg.make.clean-target",
	                                          default_clean_target);
	const char* ignore_clean_failure_var =
		dictionary_get(pinfo, "pkg.make.ignore-clean-failure", "true");
	bool ignore_clean_failure = parse_boolean(ignore_clean_failure_var);

	if ( SHOULD_DO_BUILD_STEP(BUILD_STEP_PRE_CLEAN, minfo) && !use_build_dir )
		Make(minfo, clean_target, NULL, !ignore_clean_failure);

	// Configure the build directory if needed.
	if ( strcmp(build_system, "configure") == 0 &&
	     SHOULD_DO_BUILD_STEP(BUILD_STEP_CONFIGURE, minfo) )
		Configure(minfo);

	bool location_independent =
		parse_boolean(dictionary_get(pinfo, "pkg.location-independent", "false"));

	const char* subdir = dictionary_get(pinfo, "pkg.subdir", NULL);

	const char* build_target = dictionary_get(pinfo, "pkg.make.build-target", "all");
	const char* install_target = dictionary_get(pinfo, "pkg.make.install-target", "install");

	if ( !location_independent && !minfo->prefix )
		error(1, 0, "error: %s is not location independent and you need to "
		            "specify the intended destination prefix using --prefix or "
		            "PREFIX", minfo->package_name);

	if ( SHOULD_DO_BUILD_STEP(BUILD_STEP_BUILD, minfo) )
		Make(minfo, build_target, NULL, true, subdir);

	char* tardir_rel = print_string("%s/%s", minfo->tmp, "tmp-tixbuild");
	char* destdir_rel = print_string("%s/%s", minfo->tmp, "tmp-tixbuild/data");
	char* tixdir_rel = print_string("%s/%s", minfo->tmp, "tmp-tixbuild/tix");
	char* tixinfo_rel = print_string("%s/%s", minfo->tmp, "tmp-tixbuild/tix/tixinfo");

	while ( mkdir(tardir_rel, 0777) != 0 )
	{
		if ( errno != EEXIST )
			error(1, errno, "mkdir: `%s'", tardir_rel);
		if ( rmdir(tardir_rel) != 0 )
			error(1, errno, "rmdir: `%s'", tardir_rel);
	}

	if ( mkdir(destdir_rel, 0777) != 0 )
		error(1, errno, "mkdir: `%s'", destdir_rel);
	if ( mkdir(tixdir_rel, 0777) != 0 )
		error(1, errno, "mkdir: `%s'", tixdir_rel);

	char* destdir = canonicalize_file_name(destdir_rel);
	if ( !destdir )
		error(1, errno, "canonicalize_file_name: `%s'", destdir_rel);

	if ( SHOULD_DO_BUILD_STEP(BUILD_STEP_INSTALL, minfo) )
		Make(minfo, install_target, destdir, true, subdir);

	const char* post_install_cmd = dictionary_get(pinfo, "pkg.post-install.cmd");

	if ( post_install_cmd &&
	     SHOULD_DO_BUILD_STEP(BUILD_STEP_POST_INSTALL, minfo) &&
	     fork_and_wait_or_recovery() )
	{
		if ( chdir(minfo->package_dir) != 0 )
			error(1, errno, "chdir: `%s'", minfo->package_dir);
		setenv("TIX_BUILD_DIR", minfo->build_dir, 1);
		setenv("TIX_SOURCE_DIR", minfo->package_dir, 1);
		setenv("TIX_INSTALL_DIR", destdir, 1);
		if ( minfo->sysroot )
			setenv("TIX_SYSROOT", minfo->sysroot, 1);
		else
			unsetenv("TIX_SYSROOT");
		setenv("BUILD", minfo->build, 1);
		setenv("HOST", minfo->host, 1);
		setenv("TARGET", minfo->target, 1);
		if ( minfo->prefix )
			setenv("PREFIX", minfo->prefix, 1);
		else
			unsetenv("PREFIX");
		if ( minfo->exec_prefix )
			setenv("EXEC_PREFIX", minfo->exec_prefix, 1);
		else
			unsetenv("EXEC_PREFIX");
		const char* cmd_argv[] =
		{
			post_install_cmd,
			NULL
		};
		recovery_execvp(cmd_argv[0], (char* const*) cmd_argv);
		error(127, errno, "%s", cmd_argv[0]);
	}

	const char* tix_ext = ".tix.tar.xz";
	char* package_tix = print_string("%s/%s%s", minfo->destination,
	                                 minfo->package_name, tix_ext);

	FILE* tixinfo_fp = fopen(tixinfo_rel, "w");
	if ( !tixinfo_fp )
		error(1, errno, "`%s'", tixinfo_rel);

	const char* runtime_deps = dictionary_get(pinfo, "pkg.runtime-deps");

	fprintf(tixinfo_fp, "tix.version=1\n");
	fprintf(tixinfo_fp, "tix.class=tix\n");
	fprintf(tixinfo_fp, "tix.platform=%s\n", minfo->host);
	fprintf(tixinfo_fp, "pkg.name=%s\n", minfo->package_name);
	if ( runtime_deps )
		fprintf(tixinfo_fp, "pkg.runtime-deps=%s\n", runtime_deps);
	if ( location_independent )
		fprintf(tixinfo_fp, "pkg.location-independent=true\n");
	else
		fprintf(tixinfo_fp, "pkg.prefix=%s\n", minfo->prefix);

	if ( ferror(tixinfo_fp) )
		error(1, errno, "write: `%s'", tixinfo_rel);

	fclose(tixinfo_fp);

	if ( SHOULD_DO_BUILD_STEP(BUILD_STEP_PACKAGE, minfo) )
	{
		printf("Creating `%s'...\n", package_tix);
		if ( fork_and_wait_or_recovery() )
		{
			const char* cmd_argv[] =
			{
				minfo->tar,
				"-C", tardir_rel,
				"--remove-files",
				"--create",
				"--xz",
				"--file", package_tix,
				"tix",
				"data",
				NULL
			};
			recovery_execvp(cmd_argv[0], (char* const*) cmd_argv);
			error(127, errno, "%s", cmd_argv[0]);
		}
	}

	unlink(tixinfo_rel);
	rmdir(destdir_rel);
	rmdir(tixdir_rel);
	rmdir(tardir_rel);

	free(tardir_rel);
	free(destdir_rel);
	free(tixdir_rel);
	free(tixinfo_rel);

	free(package_tix);

	// Clean the build directory after the successful build.
	if ( SHOULD_DO_BUILD_STEP(BUILD_STEP_POST_CLEAN, minfo) )
		Make(minfo, clean_target, NULL, !ignore_clean_failure);
}

void VerifySourceTixInformation(metainfo_t* minfo)
{
	const char* pipath = minfo->package_info_path;
	string_array_t* pinfo = &minfo->package_info;
	const char* tix_version = VerifyInfoVariable(pinfo, "tix.version", pipath);
	if ( atoi(tix_version) != 1 )
		error(1, 0, "error: `%s': tix version `%s' not supported", pipath,
		            tix_version);
	const char* tix_class = VerifyInfoVariable(pinfo, "tix.class", pipath);
	if ( !strcmp(tix_class, "tix") )
		error(1, 0, "error: `%s': this object is a binary tix and is already "
		            "compiled.\n", pipath);
	if ( strcmp(tix_class, "srctix") )
		error(1, 0, "error: `%s': tix class `%s' is not `srctix': this object "
		            "is not suitable for compilation.", pipath, tix_class);
	VerifyInfoVariable(pinfo, "pkg.name", pipath);
	VerifyInfoVariable(pinfo, "pkg.build-system", pipath);
}

// TODO: The MAKEFLAGS variable is actually not in the same format as the token
//       string language. It appears that GNU make doesn't escape " characters,
//       but instead consider them normal characters. This should work as
//       expected, though, as long as the MAKEFLAGS variable doesn't contain any
//       quote characters.
void PurifyMakeflags()
{
	const char* makeflags_environment = getenv("MAKEFLAGS");
	if ( !makeflags_environment )
		return;
	string_array_t makeflags = string_array_make();
	string_array_append_token_string(&makeflags, makeflags_environment);
	// Discard all the environmental variables in MAKEFLAGS.
	for ( size_t i = 0; i < makeflags.length; i++ )
	{
		char* flag = makeflags.strings[i];
		assert(flag);
		if ( flag[0] == '-' )
			continue;
		if ( !strchr(flag, '=') )
			continue;
		free(flag);
		for ( size_t n = i + 1; n < makeflags.length; n++ )
			makeflags.strings[n-1] = makeflags.strings[n];
		makeflags.length--;
		i--;
	}
	char* new_makeflags_environment = token_string_of_string_array(&makeflags);
	assert(new_makeflags_environment);
	setenv("MAKEFLAGS", new_makeflags_environment, 1);
	free(new_makeflags_environment);
	string_array_reset(&makeflags);
}

static void help(FILE* fp, const char* argv0)
{
	fprintf(fp, "Usage: %s [OPTION]... PACKAGE\n", argv0);
	fprintf(fp, "Compile a source tix into a tix suitable for installation.\n");
}

static void version(FILE* fp, const char* argv0)
{
	fprintf(fp, "%s (Sortix) %s\n", argv0, VERSIONSTR);
	fprintf(fp, "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n");
	fprintf(fp, "This is free software: you are free to change and redistribute it.\n");
	fprintf(fp, "There is NO WARRANTY, to the extent permitted by law.\n");
}

int main(int argc, char* argv[])
{
	PurifyMakeflags();

	metainfo_t minfo;
	minfo.build = NULL;
	minfo.destination = strdup(getenv_def("TIX_BUILD_DESTINATION", "."));
	minfo.host = NULL;
	minfo.makeflags = strdup_null(getenv_def("MAKEFLAGS", NULL));
	minfo.make = strdup(getenv_def("MAKE", "make"));
	minfo.prefix = strdup_null(getenv_def("PREFIX", NULL));
	minfo.exec_prefix = strdup_null(getenv_def("EXEC_PREFIX", NULL));
	minfo.sysroot = strdup_null(getenv_def("SYSROOT", NULL));
	minfo.target = NULL;
	minfo.tar = strdup(getenv_def("TAR", "tar"));
	minfo.tmp = strdup(getenv_def("BUILDTMP", "."));
	char* start_step_string = strdup("start");
	char* end_step_string = strdup("end");

	const char* argv0 = argv[0];
	for ( int i = 0; i < argc; i++ )
	{
		const char* arg = argv[i];
		if ( arg[0] != '-' || !arg[1] )
			continue;
		argv[i] = NULL;
		if ( !strcmp(arg, "--") )
			break;
		if ( arg[1] != '-' )
		{
			while ( char c = *++arg ) switch ( c )
			{
			default:
				fprintf(stderr, "%s: unknown option -- '%c'\n", argv0, c);
				help(stderr, argv0);
				exit(1);
			}
		}
		else if ( !strcmp(arg, "--help") )
			help(stdout, argv0), exit(0);
		else if ( !strcmp(arg, "--version") )
			version(stdout, argv0), exit(0);
		else if ( GET_OPTION_VARIABLE("--build", &minfo.build) ) { }
		else if ( GET_OPTION_VARIABLE("--destination", &minfo.destination) ) { }
		else if ( GET_OPTION_VARIABLE("--end", &end_step_string) ) { }
		else if ( GET_OPTION_VARIABLE("--host", &minfo.host) ) { }
		else if ( GET_OPTION_VARIABLE("--makeflags", &minfo.makeflags) ) { }
		else if ( GET_OPTION_VARIABLE("--make", &minfo.make) ) { }
		else if ( GET_OPTION_VARIABLE("--prefix", &minfo.prefix) ) { }
		else if ( GET_OPTION_VARIABLE("--exec-prefix", &minfo.exec_prefix) ) { }
		else if ( GET_OPTION_VARIABLE("--start", &start_step_string) ) { }
		else if ( GET_OPTION_VARIABLE("--sysroot", &minfo.sysroot) ) { }
		else if ( GET_OPTION_VARIABLE("--target", &minfo.target) ) { }
		else if ( GET_OPTION_VARIABLE("--tar", &minfo.tar) ) { }
		else if ( GET_OPTION_VARIABLE("--tmp", &minfo.tmp) ) { }
		else
		{
			fprintf(stderr, "%s: unknown option: %s\n", argv0, arg);
			help(stderr, argv0);
			exit(1);
		}
	}

	if ( !(minfo.start_step = step_of_step_name(start_step_string)) )
	{
		fprintf(stderr, "%s: no such step `%s'\n", argv0, start_step_string);
		exit(1);
	}

	if ( !(minfo.end_step = step_of_step_name(end_step_string)) )
	{
		fprintf(stderr, "%s: no such step `%s'\n", argv0, end_step_string);
		exit(1);
	}

	if ( argc == 1 )
	{
		help(stdout, argv0);
		exit(0);
	}

	compact_arguments(&argc, &argv);

	if ( minfo.prefix && !strcmp(minfo.prefix, "/") )
		minfo.prefix[0] = '\0';

	if ( argc < 2 )
	{
		fprintf(stderr, "%s: no package specified\n", argv0);
		exit(1);
	}

	if ( 2 < argc )
	{
		fprintf(stderr, "%s: unexpected extra operand `%s'\n", argv0, argv[2]);
		exit(1);
	}

	minfo.package_dir = canonicalize_file_name(argv[1]);
	if ( !minfo.package_dir )
		error(1, errno, "canonicalize_file_name: `%s'", argv[1]);

	if ( minfo.build && !minfo.build[0] )
		free(minfo.build), minfo.build = NULL;
	if ( minfo.host && !minfo.host[0] )
		free(minfo.host), minfo.host = NULL;
	if ( minfo.target && !minfo.target[0] )
		free(minfo.target), minfo.target = NULL;

	if ( !minfo.build && !(minfo.build = GetBuildTriplet()) )
		error(1, errno, "unable to determine build, use --build or BUILD");
	if ( !minfo.host && !(minfo.host = strdup_null_if_content(getenv("HOST"))) )
		minfo.host = strdup(minfo.build);
	if ( !minfo.target && !(minfo.target = strdup_null_if_content(getenv("TARGET"))) )
		minfo.target = strdup(minfo.host);

	if ( minfo.prefix && !minfo.exec_prefix )
	{
// TODO: After releasing Sortix 1.0, switch to this branch that defaults the
//       exec-prefix to the prefix.
#if 0
		minfo.exec_prefix = strdup(minfo.prefix);
#else // Sortix 0.9 compatibility.
		minfo.exec_prefix = print_string("%s/%s", minfo.prefix, minfo.host);
#endif
	}

	if ( !IsDirectory(minfo.package_dir) )
		error(1, errno, "`%s'", minfo.package_dir);

	minfo.package_info_path = print_string("%s/tixbuildinfo",
	                                       minfo.package_dir);

	string_array_t* package_info = &(minfo.package_info = string_array_make());
	if ( !dictionary_append_file_path(package_info, minfo.package_info_path) )
	{
		if ( errno == ENOENT )
			fprintf(stderr, "%s: `%s' doesn't appear to be a source .tix:\n",
			                argv0, minfo.package_dir);
		error(1, errno, "`%s'", minfo.package_info_path);
	}

	VerifySourceTixInformation(&minfo);
	minfo.package_name = strdup(dictionary_get(package_info, "pkg.name"));

	emit_pkg_config_wrapper(&minfo);

	BuildPackage(&minfo);

	return 0;
}
