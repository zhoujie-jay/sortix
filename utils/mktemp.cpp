/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2015.

    This program is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
    more details.

    You should have received a copy of the GNU General Public License along with
    this program. If not, see <http://www.gnu.org/licenses/>.

    mktemp.cpp
    Create temporary files and directories.

*******************************************************************************/

#include <errno.h>
#include <error.h>
#include <limits.h>
#include <locale.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

__attribute__((format(printf, 1, 2)))
char* print_string(const char* format, ...)
{
	va_list ap;
	va_start(ap, format);
	char* ret;
	if ( vasprintf(&ret, format, ap) < 0 )
		ret = NULL;
	va_end(ap);
	return ret;
}

char* join_paths(const char* a, const char* b)
{
	size_t a_len = strlen(a);
	bool has_slash = (a_len && a[a_len-1] == '/') || b[0] == '/';
	return has_slash ? print_string("%s%s", a, b) : print_string("%s/%s", a, b);
}

static void compact_arguments(int* argc, char*** argv)
{
	for ( int i = 0; i < *argc; i++ )
	{
		while ( i < *argc && !(*argv)[i] )
		{
			for ( int n = i; n < *argc; n++ )
				(*argv)[n] = (*argv)[n+1];
			(*argc)--;
		}
	}
}

static void help(FILE* fp, const char* argv0)
{
	fprintf(fp, "Usage: %s [OPTION]... [TEMPLATE]\n", argv0);
	fprintf(fp, "Create a temporary file or directory, safely, and print its name.\n");
	fprintf(fp, "TEMPLATE must contain at least 6 consecutive 'X's in last component.\n");
	fprintf(fp, "If TEMPLATE is not specified, use tmp.XXXXXX, and --tmpdir is implied.\n");
	fprintf(fp, "Files are created u+rw, and directories u+rwx, minus umask restrictions.\n");
	fprintf(fp, "\n");
	fprintf(fp, "  -d, --directory     create a directory, not a file\n");
	fprintf(fp, "  -p, --tmpdir[=DIR]  interpret TEMPLATE relative to DIR.\n");
	fprintf(fp, "  -t                  interpret TEMPLATE as a single file name component,\n");
	fprintf(fp, "                        relative to a directory.\n");
	fprintf(fp, "      --suffix=SUFF   append SUFF to TEMPLATE.  SUFF must not contain slash.\n");
	fprintf(fp, "                        This option is implied if TEMPLATE does not end in X.\n");
	fprintf(fp, "  -q, --quiet         suppress diagnostics about file/dir-creation failure\n");
	fprintf(fp, "      --help          display this help and exit\n");
	fprintf(fp, "      --version       output version information and exit\n");
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
	setlocale(LC_ALL, "");

	bool directory = false;
	bool quiet = false;
	bool rooted = false;
	const char* suffix = NULL;
	const char* tmpdir = getenv("TMPDIR");
	if ( !tmpdir )
		tmpdir = "/tmp";

	const char* argv0 = argv[0];
	for ( int i = 1; i < argc; i++ )
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
			case 'd': directory = true; break;
			case 'p':
				if ( !*(tmpdir = arg + 1) )
				{
					if ( i + 1 == argc )
					{
						error(0, 0, "option requires an argument -- 'p'");
						fprintf(stderr, "Try `%s --help' for more information.\n", argv[0]);
						exit(125);
					}
					tmpdir = argv[i+1];
					argv[++i] = NULL;
				}
				rooted = true;
				arg = "p";
				break;
			case 't': rooted = true; break;
			case 'q': quiet = true; break;
			default:
				fprintf(stderr, "%s: unknown option -- '%c'\n", argv0, c);
				help(stderr, argv0);
				exit(1);
			}
		}
		else if ( !strcmp(arg, "--directory") )
			directory = true;
		else if ( !strncmp(arg, "--suffix=", strlen("--suffix=")) )
			suffix = arg + strlen("--suffix=");
		else if ( !strcmp(arg, "--suffix") )
		{
			if ( i + 1 == argc )
			{
				error(0, 0, "option '--suffix' requires an argument");
				fprintf(stderr, "Try `%s --help' for more information.\n", argv[0]);
				exit(125);
			}
			suffix = argv[i+1];
			argv[++i] = NULL;
		}
		else if ( !strncmp(arg, "--tmpdir=", strlen("--tmpdir=")) )
		{
			tmpdir = arg + strlen("--tmpdir=");
			rooted = true;
		}
		else if ( !strcmp(arg, "--tmpdir") )
		{
			if ( i + 1 == argc )
			{
				error(0, 0, "option '--tmpdir' requires an argument");
				fprintf(stderr, "Try `%s --help' for more information.\n", argv[0]);
				exit(125);
			}
			tmpdir = argv[i+1];
			argv[++i] = NULL;
			rooted = true;
		}
		else if ( !strcmp(arg, "--help") )
			help(stdout, argv0), exit(0);
		else if ( !strcmp(arg, "--version") )
			version(stdout, argv0), exit(0);
		else
		{
			fprintf(stderr, "%s: unknown option: %s\n", argv0, arg);
			help(stderr, argv0);
			exit(1);
		}
	}

	compact_arguments(&argc, &argv);

	if ( quiet && !freopen("/dev/null", "w", stderr) )
		close(2);

	if ( 3 <= argc )
	{
		error(0, 0, "too many templates");
		fprintf(stderr, "Try `%s --help' for more information\n", argv0);
		exit(1);
	}

	// TODO: Add support for an arbitrary amount of X's in mkstemps and use it.
	//       Then increase the default below to 10 X's and in --help as well.
	const char* input_templ;
	if ( 2 <= argc )
	{
		input_templ = argv[1];
	}
	else
	{
		input_templ = "tmp.XXXXXX";
		rooted = true;
	}

	char* templ;
	if ( rooted )
	{
		templ = join_paths(tmpdir, input_templ);
		if ( !templ )
			error(1, errno, "malloc");
	}
	else
	{
		templ = strdup(input_templ);
		if ( !templ )
			error(1, errno, "malloc");
	}

	size_t suffix_length;
	if ( suffix )
	{
		suffix_length = strlen(suffix);
		if ( INT_MAX < suffix_length )
			error(1, 0, "suffix is too long");
		char* new_templ;
		if ( asprintf(&new_templ, "%s%s", templ, suffix) < 0 )
			error(1, errno, "malloc");
		free(templ);
		templ = new_templ;
	}
	else
	{
		size_t templ_length = strlen(templ);
		suffix_length = 0;
		while ( templ_length - suffix_length &&
		        templ[templ_length - suffix_length - 1] != 'X' &&
		        templ[templ_length - suffix_length - 1] != '/' )
			suffix_length++;
	}

	size_t templ_length = strlen(templ);
	size_t xcount = 0;
	while ( templ_length - (suffix_length + xcount) &&
	        templ[templ_length - (suffix_length + xcount) - 1] == 'X' )
		xcount++;

	if ( xcount < 6 )
		error(1, 0, "too few X's in template `%s'", templ);

	char* path = strdup(templ);
	if ( !path )
		error(1, errno, "strdup");
	if ( directory )
	{
		if ( !mkdtemps(path, suffix_length) )
			error(1, errno, "%s", templ);
	}
	else
	{
		if ( INT_MAX < suffix_length )
			error(1, 0, "suffix is too long");
		int fd = mkstemps(path, (int) suffix_length);
		if ( fd < 0 )
			error(1, errno, "%s", templ);
		close(fd);
	}
	puts(path);
	free(path);

	free(templ);

	if ( ferror(stdout) || fflush(stdout) == EOF )
		return 1;
	return 0;
}
