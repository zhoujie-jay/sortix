/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2014, 2015.

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

    sh.cpp
    A hacky Sortix shell.

*******************************************************************************/

#include <sys/wait.h>

#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <inttypes.h>
#include <ioleast.h>
#include <libgen.h>
#include <locale.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <wchar.h>
#include <wctype.h>

#include "editline.h"
#include "showline.h"
#include "util.h"

#if !defined(VERSIONSTR)
#define VERSIONSTR "unknown version"
#endif

const char* builtin_commands[] =
{
	"cd",
	"exit",
	"unset",
	"clearenv",
	(const char*) NULL,
};

int status = 0;

static bool is_proper_absolute_path(const char* path)
{
	if ( path[0] == '\0' )
		return false;
	if ( path[0] != '/' )
		return false;
	while ( path[0] )
	{
		if ( path[0] == '/' )
			path++;
		else if ( path[0] == '.' &&
		          (path[1] == '\0' || path[1] == '/') )
			return false;
		else if ( path[0] == '.' &&
		          path[1] == '.' &&
		          (path[2] == '\0' || path[2] == '/') )
			return false;
		else
		{
			while ( *path && *path != '/' )
				path++;
		}
	}
	return true;
}

void update_env()
{
	char str[128];
	struct winsize ws;
	if ( tcgetwinsize(0, &ws) == 0 )
	{
		snprintf(str, sizeof(str), "%zu", ws.ws_col);
		setenv("COLUMNS", str, 1);
		snprintf(str, sizeof(str), "%zu", ws.ws_row);
		setenv("LINES", str, 1);
	}
}

bool matches_simple_pattern(const char* string, const char* pattern)
{
	size_t wildcard_index = strcspn(pattern, "*");
	if ( !pattern[wildcard_index] )
		return strcmp(string, pattern) == 0;
	if ( pattern[0] == '*' && string[0] == '.' )
		return false;
	size_t string_length = strlen(string);
	size_t pattern_length = strlen(pattern);
	size_t pattern_last = pattern_length - (wildcard_index + 1);
	return strncmp(string, pattern, wildcard_index) == 0 &&
	       strcmp(string + string_length - pattern_last,
	              pattern + wildcard_index + 1) == 0;
}

enum sh_tokenize_result
{
	SH_TOKENIZE_RESULT_OK,
	SH_TOKENIZE_RESULT_PARTIAL,
	SH_TOKENIZE_RESULT_INVALID,
	SH_TOKENIZE_RESULT_ERROR,
};

enum sh_tokenize_result sh_tokenize(const char* command,
                                    char*** tokens_ptr,
                                    size_t* tokens_used_ptr,
                                    size_t* tokens_length_ptr)
{
	enum sh_tokenize_result result = SH_TOKENIZE_RESULT_OK;

	char** tokens = NULL;
	size_t tokens_used = 0;
	size_t tokens_length = 0;

	size_t command_index = 0;
	while ( true )
	{
		if ( command[command_index] == '\0' )
			break;

		if ( isspace((unsigned char) command[command_index]) )
		{
			command_index++;
			continue;
		}

		if ( command[command_index] == '#' )
		{
			while ( command[command_index] != '\0' &&
			        command[command_index] != '\n' )
				command_index++;
			continue;
		}

		size_t token_start = command_index;
		bool escaped = false;
		bool stop = false;
		while ( true )
		{
			if ( command[command_index] == '\0' )
			{
				if ( escaped )
					result = SH_TOKENIZE_RESULT_PARTIAL;
				stop = true;
				break;
			}
			else if ( !escaped && command[command_index] == '\\' )
			{
				escaped = true;
				command_index++;
			}
			else if ( !escaped && isspace((unsigned char) command[command_index]) )
			{
				break;
			}
			else
			{
				command_index++;
				escaped = false;
			}
		}

		if ( tokens_used == tokens_length )
		{
			size_t new_length = tokens_length ? 2 * tokens_length : 16;
			size_t new_size = new_length * sizeof(char*);
			char** new_tokens = (char**) realloc(tokens, new_size);
			if ( !new_tokens )
			{
				result = SH_TOKENIZE_RESULT_ERROR;
				break;
			}
			tokens_length = new_length;
			tokens = new_tokens;
		}

		size_t token_length = command_index - token_start;
		char* token = strndup(command + token_start, token_length);
		if ( !token )
		{
			result = SH_TOKENIZE_RESULT_ERROR;
			break;
		}

		tokens[tokens_used++] = token;

		if ( stop )
			break;
	}

	*tokens_ptr = tokens;
	*tokens_used_ptr = tokens_used;
	*tokens_length_ptr = tokens_length;

	return result;
}

bool is_shell_input_ready(const char* input)
{
	char** tokens = NULL;
	size_t tokens_used = 0;
	size_t tokens_length = 0;

	enum sh_tokenize_result tokenize_result =
		sh_tokenize(input, &tokens, &tokens_used, &tokens_length);

	bool result = tokenize_result == SH_TOKENIZE_RESULT_OK;

	for ( size_t i = 0; i < tokens_used; i++ )
		free(tokens[i]);
	free(tokens);

	return result;
}

int lexical_chdir(char* path)
{
	assert(path[0] == '/');

	int fd = open("/", O_RDONLY | O_DIRECTORY);
	if ( fd < 0 )
		return -1;

	size_t input_index = 1;
	size_t output_index = 1;

	while ( path[input_index] )
	{
		if ( path[input_index] == '/' )
		{
			if ( output_index && path[output_index-1] != '/' )
				path[output_index++] = path[input_index];
			input_index++;
			continue;
		}

		char* elem = path + input_index;
		size_t elem_length = strcspn(elem, "/");
		char lc = elem[elem_length];
		elem[elem_length] = '\0';

		if ( !strcmp(elem, ".") )
		{
			elem[elem_length] = lc;
			input_index += elem_length;
			continue;
		}

		if ( !strcmp(elem, "..") )
		{
			elem[elem_length] = lc;
			input_index += elem_length;
			if ( 2 <= output_index && path[output_index-1] == '/' )
				output_index--;
			while ( 2 <= output_index && path[output_index-1] != '/' )
				output_index--;
			if ( 2 <= output_index && path[output_index-1] == '/' )
				output_index--;
			lc = path[output_index];
			path[output_index] = '\0';
			int new_fd = open(path, O_RDONLY | O_DIRECTORY);
			close(fd);
			if ( new_fd < 0 )
				return -1;
			fd = new_fd;
			path[output_index] = lc;
			continue;
		}

		if ( 0 <= fd )
		{
			int new_fd = openat(fd, elem, O_RDONLY | O_DIRECTORY);
			if ( new_fd < 0 )
				close(fd);
			fd = new_fd;
		}

		for ( size_t i = 0; i < elem_length; i++ )
			path[output_index++] = path[input_index++];

		elem[elem_length] = lc;
	}

	path[output_index] = '\0';
	if ( 2 <= output_index && path[output_index-1] == '/' )
		path[--output_index] = '\0';

	int fchdir_ret = fchdir(fd);
	close(fd);
	if ( fchdir_ret < 0 )
		return -1;

	unsetenv("PWD");
	setenv("PWD", path, 1);

	return 0;
}

int perform_chdir(const char* path)
{
	if ( !path[0] )
		return errno = ENOENT, -1;

	char* lexical_path = NULL;
	if ( path[0] == '/' )
		lexical_path = strdup(path);
	else
	{
		char* current_pwd = get_current_dir_name();
		if ( current_pwd )
		{
			assert(current_pwd[0] == '/');
			asprintf(&lexical_path, "%s/%s", current_pwd, path);
			free(current_pwd);
		}
		else if ( getenv("PWD") )
		{
			asprintf(&lexical_path, "/%s/%s", getenv("PWD"), path);
		}
	}

	if ( lexical_path )
	{
		int ret = lexical_chdir(lexical_path);
		free(lexical_path);
		if ( ret == 0 )
			return 0;
	}

	return chdir(path);
}

int runcommandline(const char** tokens, bool* script_exited, bool interactive)
{
	int result = 127;
	size_t cmdnext = 0;
	size_t cmdstart;
	size_t cmdend;
	bool lastcmd = false;
	int pipein = 0;
	int pipeout = 1;
	int pipeinnext = 0;
	char** argv;
	size_t cmdlen;
	const char* execmode;
	const char* outputfile;
	pid_t childpid;
	pid_t pgid = -1;
	bool internal;
	int internalresult;
	size_t num_tokens = 0;
	while ( tokens[num_tokens] )
		num_tokens++;
readcmd:
	// Collect any pending zombie processes.
	while ( 0 < waitpid(-1, NULL, WNOHANG) );

	cmdstart = cmdnext;
	for ( cmdend = cmdstart; true; cmdend++ )
	{
		const char* token = tokens[cmdend];
		if ( !token ||
		     strcmp(token, ";") == 0 ||
		     strcmp(token, "&") == 0 ||
		     strcmp(token, "|") == 0 ||
		     strcmp(token, ">") == 0 ||
		     strcmp(token, ">>") == 0 ||
		     false )
		{
			break;
		}
	}

	cmdlen = cmdend - cmdstart;
	if ( !cmdlen ) { fprintf(stderr, "expected command\n"); goto out; }
	execmode = tokens[cmdend];
	if ( !execmode ) { lastcmd = true; execmode = ";"; }
	tokens[cmdend] = NULL;

	if ( strcmp(execmode, "|") == 0 )
	{
		int pipes[2];
		if ( pipe(pipes) ) { perror("pipe"); goto out; }
		if ( pipeout != 1 ) { close(pipeout); } pipeout = pipes[1];
		if ( pipeinnext != 0 ) { close(pipeinnext); } pipeinnext = pipes[0];
	}

	outputfile = NULL;
	if ( strcmp(execmode, ">") == 0 || strcmp(execmode, ">>") == 0 )
	{
		outputfile = tokens[cmdend+1];
		if ( !outputfile ) { fprintf(stderr, "expected filename\n"); goto out; }
		const char* nexttok = tokens[cmdend+2];
		if ( nexttok ) { fprintf(stderr, "too many filenames\n"); goto out; }
	}

	for ( size_t i = cmdstart; i < cmdend; i++ )
	{
		const char* pattern = tokens[i];
		size_t wildcard_pos = strcspn(pattern, "*");
		if ( !pattern[wildcard_pos] )
			continue;
		bool found_slash = false;
		size_t last_slash = 0;
		for ( size_t n = 0; n < wildcard_pos; n++ )
			if ( pattern[n] == '/' )
				last_slash = n, found_slash = true;
		size_t match_from = found_slash ? last_slash + 1 : 0;
		DIR* dir;
		size_t pattern_prefix = 0;
		if ( !found_slash )
		{
			if ( !(dir = opendir(".")) )
				continue;
		}
		else
		{
			char* dirpath = strdup(pattern);
			if ( !dirpath )
				continue;
			dirpath[last_slash] = '\0';
			pattern_prefix = last_slash + 1;
			dir = opendir(dirpath);
			free(dirpath);
			if ( !dir )
				continue;
		}
		size_t num_inserted = 0;
		size_t last_inserted_index = i;
		while ( struct dirent* entry = readdir(dir) )
		{
			if ( !matches_simple_pattern(entry->d_name, pattern + match_from) )
				continue;
			// TODO: Memory leak.
			char* name = (char*) malloc(pattern_prefix + strlen(entry->d_name) + 1);
			memcpy(name, pattern, pattern_prefix);
			strcpy(name + pattern_prefix, entry->d_name);
			if ( !name )
				continue;
			if ( num_inserted )
			{
				// TODO: Reckless modification of the tokens array.
				for ( size_t n = num_tokens; n != last_inserted_index; n-- )
					tokens[n+1] = tokens[n];
				num_tokens++;
				cmdend++;
			}
			// TODO: Reckless modification of the tokens array.
			tokens[last_inserted_index = i + num_inserted++] = name;
		}
		closedir(dir);
	}

	cmdnext = cmdend + 1;
	argv = (char**) (tokens + cmdstart);

	update_env();
	char statusstr[32];
	snprintf(statusstr, sizeof(statusstr), "%i", status);
	setenv("?", statusstr, 1);

	for ( char** argp = argv; *argp; argp++ )
	{
		char* arg = *argp;
		if ( arg[0] != '$' )
			continue;
		arg = getenv(arg+1);
		if ( !arg )
			arg = (char*) "";
		*argp = arg;
	}

	internal = false;
	internalresult = 0;
	if ( strcmp(argv[0], "cd") == 0 )
	{
		internal = true;
		const char* newdir = getenv_safe("HOME", "/");
		if ( argv[1] )
			newdir = argv[1];
		if ( perform_chdir(newdir) )
		{
			error(0, errno, "cd: %s", newdir);
			internalresult = 1;
		}
	}
	if ( strcmp(argv[0], "exit") == 0 )
	{
		int exitcode = argv[1] ? atoi(argv[1]) : 0;
		*script_exited = true;
		return exitcode;
	}
	if ( strcmp(argv[0], "unset") == 0 )
	{
		internal = true;
		unsetenv(argv[1] ? argv[1] : "");
	}
	if ( strcmp(argv[0], "clearenv") == 0 )
	{
		internal = true;
		clearenv();
	}

	childpid = internal ? getpid() : fork();
	if ( childpid < 0 ) { perror("fork"); goto out; }
	if ( childpid )
	{
		if ( !internal )
		{
			if ( pgid == -1 )
				pgid = childpid;
			setpgid(childpid, pgid);
		}

		if ( pipein != 0 ) { close(pipein); pipein = 0; }
		if ( pipeout != 1 ) { close(pipeout); pipeout = 1; }
		if ( pipeinnext != 0 ) { pipein = pipeinnext; pipeinnext = 0; }

		if ( strcmp(execmode, "&") == 0 && !tokens[cmdnext] )
		{
			result = 0; goto out;
		}

		if ( strcmp(execmode, "&") == 0 )
			pgid = -1;

		if ( strcmp(execmode, "&") == 0 || strcmp(execmode, "|") == 0 )
		{
			goto readcmd;
		}

		status = internalresult;
		int exitstatus;
		tcsetpgrp(0, pgid);
		if ( !internal && waitpid(childpid, &exitstatus, 0) < 0 )
		{
			perror("waitpid");
			return 127;
		}
		tcsetpgrp(0, getpgid(0));

		// TODO: HACK: Most signals can't kill processes yet.
		if ( WEXITSTATUS(exitstatus) == 128 + SIGINT )
			printf("^C\n");
		if ( WTERMSIG(status) == SIGKILL )
			printf("Killed\n");

		status = WEXITSTATUS(exitstatus);

		if ( strcmp(execmode, ";") == 0 && tokens[cmdnext] && !lastcmd )
		{
			goto readcmd;
		}

		result = status;
		goto out;
	}

	setpgid(0, pgid != -1 ? pgid : 0);

	if ( pipeinnext != 0 ) { close(pipeinnext); }

	if ( pipein != 0 )
	{
		close(0);
		dup(pipein);
		close(pipein);
	}

	if ( pipeout != 1 )
	{
		close(1);
		dup(pipeout);
		close(pipeout);
	}

	if ( outputfile )
	{
		close(1);
		// TODO: Is this logic right or wrong?
		int flags = O_CREAT | O_WRONLY;
		if ( strcmp(execmode, ">") == 0 )
			flags |= O_TRUNC;
		if ( strcmp(execmode, ">>") == 0 )
			flags |= O_APPEND;
		if ( open(outputfile, flags, 0666) < 0 )
		{
			error(127, errno, "%s", outputfile);
		}
	}

	execvp(argv[0], argv);
	if ( interactive && errno == ENOENT )
	{
		int errno_saved = errno;
		execlp("command-not-found", "command-not-found", argv[0], NULL);
		errno = errno_saved;
	}
	error(127, errno, "%s", argv[0]);
	return 127;

out:
	if ( pipein != 0 ) { close(pipein); }
	if ( pipeout != 1 ) { close(pipeout); }
	if ( pipeinnext != 0 ) { close(pipeout); }
	return result;
}

int run_command(char* command,
                bool interactive,
                bool exit_on_error,
                bool* script_exited)
{
	size_t commandused = strlen(command);

	if ( command[0] == '\0' )
		return status;

	if ( strchr(command, '=') && !strchr(command, ' ') && !strchr(command, '\t') )
	{
		const char* key = command;
		char* equal = strchr(command, '=');
		*equal = '\0';
		const char* value = equal + 1;
		if ( setenv(key, value, 1) < 0 )
			error(1, errno, "setenv");
		return status = 0;
	}

	int argc = 0;
	const size_t ARGV_MAX_LENGTH = 2048;
	const char* argv[ARGV_MAX_LENGTH];
	argv[0] = NULL;

	bool lastwasspace = true;
	bool escaped = false;
	for ( size_t i = 0; i <= commandused; i++ )
	{
		switch ( command[i] )
		{
			case '\\':
				if ( !escaped )
				{
					memmove(command + i, command + i + 1, commandused+1 - (i-1));
					i--;
					commandused--;
					escaped = true;
					break;
				}
			case '\0':
			case ' ':
			case '\t':
			case '\n':
				if ( !command[i] || !escaped )
				{
					command[i] = 0;
					lastwasspace = true;
					break;
				}
			default:
				escaped = false;
				if ( lastwasspace )
				{
					if ( argc == ARGV_MAX_LENGTH  )
					{
						fprintf(stderr, "argv max length of %zu entries hit!\n",
						        ARGV_MAX_LENGTH);
						abort();
					}
					argv[argc++] = command + i;
				}
				lastwasspace = false;
		}
	}

	if ( !argv[0] )
		return status;

	argv[argc] = NULL;
	status = runcommandline(argv, script_exited, interactive);
	if ( status && exit_on_error )
		*script_exited = true;
	return status;
}

bool does_line_editing_need_another_line(void*, const char* line)
{
	return !is_shell_input_ready(line);
}

bool is_outermost_shell()
{
	const char* shlvl_str = getenv("SHLVL");
	if ( !shlvl_str )
		return true;
	return atol(shlvl_str) <= 1;
}

void on_trap_eof(void* edit_state_ptr)
{
	if ( is_outermost_shell() )
		return;
	struct edit_line* edit_state = (struct edit_line*) edit_state_ptr;
	edit_line_type_codepoint(edit_state, L'e');
	edit_line_type_codepoint(edit_state, L'x');
	edit_line_type_codepoint(edit_state, L'i');
	edit_line_type_codepoint(edit_state, L't');
}

bool is_usual_char_for_completion(char c)
{
	return !isspace((unsigned char) c) &&
	       c != ';' && c != '&' && c != '|' &&
	       c != '<' && c != '>' && c != '#' && c != '$';
}

size_t do_complete(char*** completions_ptr,
                   size_t* used_before_ptr,
                   size_t* used_after_ptr,
                   void*,
                   const char* partial,
                   size_t complete_at)
{
	size_t used_before = 0;
	size_t used_after = 0;

	while ( complete_at - used_before &&
	        is_usual_char_for_completion(partial[complete_at - (used_before+1)]) )
		used_before++;

#if 0
	while ( partial[complete_at + used_after] &&
	        is_usual_char_for_completion(partial[complete_at + used_after]) )
		used_after++;
#endif

	enum complete_type
	{
		COMPLETE_TYPE_FILE,
		COMPLETE_TYPE_EXECUTABLE,
		COMPLETE_TYPE_DIRECTORY,
		COMPLETE_TYPE_PROGRAM,
		COMPLETE_TYPE_VARIABLE,
	};

	enum complete_type complete_type = COMPLETE_TYPE_FILE;

	if ( complete_at - used_before && partial[complete_at - used_before-1] == '$' )
	{
		complete_type = COMPLETE_TYPE_VARIABLE;
		used_before++;
	}
	else
	{
		size_t type_offset = complete_at - used_before;
		while ( type_offset && isspace((unsigned char) partial[type_offset-1]) )
			type_offset--;

		if ( 2 <= type_offset &&
		     strncmp(partial + type_offset - 2, "cd", 2) == 0 &&
		     (type_offset == 2 || !is_usual_char_for_completion(partial[type_offset-2-1])) )
			complete_type = COMPLETE_TYPE_DIRECTORY;
		else if ( !type_offset ||
			      partial[type_offset-1] == ';' ||
			      partial[type_offset-1] == '&' ||
			      partial[type_offset-1] == '|' )
		{
			if ( memchr(partial + complete_at - used_before, '/', used_before) )
				complete_type = COMPLETE_TYPE_EXECUTABLE;
			else
				complete_type = COMPLETE_TYPE_PROGRAM;
		}
	}

	// TODO: Use reallocarray.
	char** completions = (char**) malloc(sizeof(char**) * 1024 /* TODO: HARD-CODED! */);
	size_t num_completions = 0;

	if ( complete_type == COMPLETE_TYPE_PROGRAM ) do
	{
		for ( size_t i = 0; builtin_commands[i]; i++ )
		{
			const char* builtin = builtin_commands[i];
			if ( strncmp(builtin, partial + complete_at - used_before, used_before) != 0 )
				continue;
			// TODO: Add allocation check!
			completions[num_completions++] = strdup(builtin + used_before);
		}
		char* path = strdup_safe(getenv("PATH"));
		if ( !path )
		{
			complete_type = COMPLETE_TYPE_FILE;
			break;
		}
		char* path_input = path;
		char* component;
		while ( (component = strsep(&path_input, ":")) )
		{
			if ( DIR* dir = opendir(component) )
			{
				while ( struct dirent* entry = readdir(dir) )
				{
					if ( strncmp(entry->d_name, partial + complete_at - used_before, used_before) != 0 )
						continue;
					if ( used_before == 0 &&  entry->d_name[0] == '.' )
						continue;
					// TODO: Add allocation check!
					completions[num_completions++] = strdup(entry->d_name + used_before);
				}
				closedir(dir);
			}
		}
		free(path);
	} while ( false );

	if ( complete_type == COMPLETE_TYPE_FILE ||
	     complete_type == COMPLETE_TYPE_EXECUTABLE ||
	     complete_type == COMPLETE_TYPE_DIRECTORY ) do
	{
		const char* pattern = partial + complete_at - used_before;
		size_t pattern_length = used_before;

		char* dirpath_alloc = NULL;
		const char* dirpath;
		if ( !memchr(pattern, '/', pattern_length) )
			dirpath = ".";
		else if ( pattern_length && pattern[pattern_length-1] == '/' )
		{
			dirpath_alloc = strndup(pattern, pattern_length);
			if ( !dirpath_alloc )
				break;
			dirpath = dirpath_alloc;
			pattern += pattern_length;
			pattern_length = 0;
		}
		else
		{
			dirpath_alloc = strndup(pattern, pattern_length);
			if ( !dirpath_alloc )
				break;
			dirpath = dirname(dirpath_alloc);
			const char* last_slash = (const char*) memrchr(pattern, '/', pattern_length);
			size_t last_slash_offset = (uintptr_t) last_slash - (uintptr_t) pattern;
			pattern += last_slash_offset + 1;
			pattern_length -= last_slash_offset + 1;
		}
		used_before = pattern_length;
		DIR* dir = opendir(dirpath);
		if ( !dir )
		{
			free(dirpath_alloc);
			break;
		}
		while ( struct dirent* entry = readdir(dir) )
		{
			if ( strncmp(entry->d_name, pattern, pattern_length) != 0 )
				continue;
			if ( pattern_length == 0 &&  entry->d_name[0] == '.' )
				continue;
			struct stat st;
			bool is_directory = entry->d_type == DT_DIR ||
			                    (entry->d_type == DT_UNKNOWN &&
			                     !fstatat(dirfd(dir), entry->d_name, &st, 0) &&
			                     S_ISDIR(st.st_mode));
			bool is_executable = complete_type == COMPLETE_TYPE_EXECUTABLE &&
			                     !fstatat(dirfd(dir), entry->d_name, &st, 0) &&
			                     st.st_mode & 0111;
			if ( complete_type == COMPLETE_TYPE_DIRECTORY && !is_directory )
				continue;
			if ( complete_type == COMPLETE_TYPE_EXECUTABLE &&
			     !(is_directory || is_executable) )
				continue;
			size_t name_length = strlen(entry->d_name);
			char* completion = (char*) malloc(name_length - pattern_length + 1 + 1);
			if ( !completion )
				continue;
			strcpy(completion, entry->d_name + pattern_length);
			if ( is_directory )
				strcat(completion, "/");
			completions[num_completions++] = completion;
		}
		closedir(dir);
		free(dirpath_alloc);
	} while ( false );

	if ( complete_type == COMPLETE_TYPE_VARIABLE ) do
	{
		const char* pattern = partial + complete_at - used_before + 1;
		size_t pattern_length = used_before - 1;
		if ( memchr(pattern, '=', pattern_length) )
			break;
		for ( size_t i = 0; environ[i]; i++ )
		{
			if ( strncmp(pattern, environ[i], pattern_length) != 0 )
				continue;
			const char* rest = environ[i] + pattern_length;
			size_t equal_offset = strcspn(rest, "=");
			if ( rest[equal_offset] != '=' )
				continue;
			completions[num_completions++] = strndup(rest, equal_offset);
		}
	} while ( false );

	*used_before_ptr = used_before;
	*used_after_ptr = used_after;

	return *completions_ptr = completions, num_completions;
}

struct sh_read_command
{
	char* command;
	bool abort_condition;
	bool eof_condition;
	bool error_condition;
};

void read_command_interactive(struct sh_read_command* sh_read_command)
{
	update_env();

	static struct edit_line edit_state; // static to preserve command history.
	edit_state.in_fd = 0;
	edit_state.out_fd = 1;
	edit_state.check_input_incomplete_context = NULL;
	edit_state.check_input_incomplete = does_line_editing_need_another_line;
	edit_state.trap_eof_opportunity_context = &edit_state;
	edit_state.trap_eof_opportunity = on_trap_eof;
	edit_state.complete_context = NULL;
	edit_state.complete = do_complete;

	char* current_dir = get_current_dir_name();

	const char* print_username = getlogin();
	if ( !print_username )
		print_username = getuid() == 0 ? "root" : "?";
	char hostname[256];
	if ( gethostname(hostname, sizeof(hostname)) < 0 )
		strlcpy(hostname, "(none)", sizeof(hostname));
	const char* print_hostname = hostname;
	const char* print_dir = current_dir ? current_dir : "?";
	const char* home_dir = getenv_safe("HOME", "");

	const char* print_dir_1 = print_dir;
	const char* print_dir_2 = "";

	size_t home_dir_len = strlen(home_dir);
	if ( home_dir_len && strncmp(print_dir, home_dir, home_dir_len) == 0 )
	{
		print_dir_1 = "~";
		print_dir_2 = print_dir + home_dir_len;
	}

	char* ps1;
	asprintf(&ps1, "\e[32m%s@%s \e[36m%s%s #\e[37m ",
		print_username,
		print_hostname,
		print_dir_1,
		print_dir_2);

	free(current_dir);

	edit_state.ps1 = ps1;
	edit_state.ps2 = "> ";

	edit_line(&edit_state);

	free(ps1);

	if ( edit_state.abort_editing )
	{
		sh_read_command->abort_condition = true;
		return;
	}

	if ( edit_state.eof_condition )
	{
		sh_read_command->eof_condition = true;
		return;
	}

	char* command = edit_line_result(&edit_state);
	assert(command);
	for ( size_t i = 0; command[i]; i++ )
		if ( command[i + 0] == '\\' && command[i + 1] == '\n' )
			command[i + 0] = ' ',
			command[i + 1] = ' ';
	sh_read_command->command = command;
}

void read_command_non_interactive(struct sh_read_command* sh_read_command,
                                  FILE* fp)
{
	int fd = fileno(fp);

	size_t command_used = 0;
	size_t command_length = 1024;
	char* command = (char*) malloc(command_length + 1);
	if ( !command )
		error(64, errno, "malloc");
	command[0] = '\0';

	while ( true )
	{
		char c;
		if ( 0 <= fd )
		{
			ssize_t bytes_read = read(fd, &c, sizeof(c));
			if ( bytes_read < 0 )
			{
				sh_read_command->error_condition = true;
				free(command);
				return;
			}
			else if ( bytes_read == 0 )
			{
				if ( command_used == 0 )
				{
					sh_read_command->eof_condition = true;
					free(command);
					return;
				}
				else
				{
					c = '\n';
				}
			}
			else
			{
				assert(bytes_read == 1);
				if ( c == '\0' )
					continue;
			}
		}
		else
		{
			int ic = fgetc(fp);
			if ( ic == EOF && ferror(fp) )
			{
				sh_read_command->error_condition = true;
				free(command);
				return;
			}
			else if ( ic == EOF )
			{
				if ( command_used == 0 )
				{
					sh_read_command->eof_condition = true;
					free(command);
					return;
				}
				else
				{
					c = '\n';
				}
			}
			else
			{
				c = (char) (unsigned char) ic;
				if ( c == '\0' )
					continue;
			}
		}
		if ( c == '\n' && is_shell_input_ready(command) )
			break;
		if ( command_used == command_length )
		{
			size_t new_length = command_length * 2;
			char* new_command = (char*) realloc(command, new_length + 1);
			if ( !new_command )
				error(64, errno, "realloc");
			command = new_command;
			command_length  = new_length;
		}
		command[command_used++] = c;
		command[command_used] = '\0';
	}

	sh_read_command->command = command;
}

int run(FILE* fp,
        const char* fp_name,
        bool interactive,
        bool exit_on_error,
        bool* script_exited,
        int status)
{
	// TODO: The interactive read code should cope when the input is not a
	//       terminal; it should print the prompt and then read normally without
	//       any line editing features.
	if ( !isatty(fileno(fp)) )
		interactive = false;

	while ( true )
	{
		struct sh_read_command sh_read_command;
		memset(&sh_read_command, 0, sizeof(sh_read_command));

		if ( interactive )
			read_command_interactive(&sh_read_command);
		else
			read_command_non_interactive(&sh_read_command, fp);

		if ( sh_read_command.abort_condition )
			continue;

		if ( sh_read_command.eof_condition )
		{
			if ( interactive && is_outermost_shell() )
			{
				printf("Type exit to close the outermost shell.\n");
				continue;
			}
			break;
		}

		if ( sh_read_command.error_condition )
		{
			error(0, errno, "read: %s", fp_name);
			return *script_exited = true, 2;
		}

		status = run_command(sh_read_command.command, interactive,
		                     exit_on_error, script_exited);

		free(sh_read_command.command);

		if ( *script_exited || (status == 0 && exit_on_error) )
			break;
	}

	return status;
}

void compact_arguments(int* argc, char*** argv)
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

void help(FILE* fp, const char* argv0)
{
	fprintf(fp, "Usage: %s [OPTION...] [SCRIPT [ARGUMENT...]]\n", argv0);
	fprintf(fp, "  or:  %s [OPTION...] -c COMMAND [ARGUMENT...]\n", argv0);
	fprintf(fp, "  or:  %s [OPTION...] -s [ARGUMENT...]\n", argv0);
#if 0
	fprintf(fp, "  -a, +a         set -a\n");
	fprintf(fp, "  -b, +b         set -b\n");
#endif
	fprintf(fp, "  -c             execute the first operand as the command\n");
#if 0
	fprintf(fp, "  -C, +C         set -C\n");
	fprintf(fp, "  -e, +e         set -e\n");
	fprintf(fp, "  -f, +f         set -f\n");
	fprintf(fp, "  -h, +h         set -h\n");
#endif
	fprintf(fp, "  -i             shell is interactive\n");
#if 0
	fprintf(fp, "  -m, +m         set -m\n");
	fprintf(fp, "  -n, +n         set -n\n");
	fprintf(fp, "  -o OPTION      set -o OPTION\n");
	fprintf(fp, "  +o OPTION      set +o OPTION\n");
#endif
	fprintf(fp, "  -s             read commands from the standard input\n");
#if 0
	fprintf(fp, "  -u, +u         set -u\n");
	fprintf(fp, "  -v, +v         set -v\n");
	fprintf(fp, "  -x, +x         set -x\n");
#endif
	fprintf(fp, "      --help     display this help and exit\n");
	fprintf(fp, "      --version  output version information and exit\n");
}

void version(FILE* fp, const char* argv0)
{
	fprintf(fp, "%s (Sortix) %s\n", argv0, VERSIONSTR);
	fprintf(fp, "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n");
	fprintf(fp, "This is free software: you are free to change and redistribute it.\n");
	fprintf(fp, "There is NO WARRANTY, to the extent permitted by law.\n");
}

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "");

	// TODO: Canonicalize argv[0] if it contains a slash and isn't absolute?

	if ( const char* env_pwd = getenv("PWD") )
	{
		if ( !is_proper_absolute_path(env_pwd) )
		{
			unsetenv("PWD");
			char* real_pwd = get_current_dir_name();
			if ( real_pwd )
				setenv("PWD", real_pwd, 1);
			free(real_pwd);
		}
	}

	bool flag_c_first_operand_is_command = false;
	bool flag_e_exit_on_error = false;
	bool flag_i_interactive = false;
	bool flag_s_stdin = false;

	const char* argv0 = argv[0];
	for ( int i = 1; i < argc; i++ )
	{
		const char* arg = argv[i];
		if ( (arg[0] != '-' && arg[0] != '+') || !arg[1] )
			break;
		argv[i] = NULL;
		if ( !strcmp(arg, "--") )
			break;
		if ( arg[0] == '+' )
		{
			while ( char c = *++arg ) switch ( c )
			{
			case 'e': flag_e_exit_on_error = false; break;
			default:
				fprintf(stderr, "%s: unknown option -- '%c'\n", argv0, c);
				help(stderr, argv0);
				exit(1);
			}
		}
		else if ( arg[1] != '-' )
		{
			while ( char c = *++arg ) switch ( c )
			{
			case 'c': flag_c_first_operand_is_command = true; break;
			case 'e': flag_e_exit_on_error = true; break;
			case 'i': flag_i_interactive = true; break;
			case 's': flag_s_stdin = true; break;
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
		else
		{
			fprintf(stderr, "%s: unknown option: %s\n", argv0, arg);
			help(stderr, argv0);
			exit(1);
		}
	}

	compact_arguments(&argc, &argv);

	if ( getenv("SHLVL") )
	{
		long shlvl = atol(getenv("SHLVL"));
		if ( shlvl < 1 )
			shlvl = 1;
		else
			shlvl++;
		char shlvl_string[sizeof(long) * 3];
		snprintf(shlvl_string, sizeof(shlvl_string), "%li", shlvl);
		setenv("SHLVL", shlvl_string, 1);
	}
	else
	{
		setenv("SHLVL", "1", 1);
	}

	char pidstr[3 * sizeof(pid_t)];
	char ppidstr[3 * sizeof(pid_t)];
	snprintf(pidstr, sizeof(pidstr), "%ji", (intmax_t) getpid());
	snprintf(ppidstr, sizeof(ppidstr), "%ji", (intmax_t) getppid());
	setenv("SHELL", argv[0], 1);
	setenv("$", pidstr, 1);
	setenv("PPID", ppidstr, 1);
	setenv("?", "0", 1);

	setenv("0", argv[0], 1);

	bool script_exited = false;
	int status = 0;

	if ( flag_c_first_operand_is_command )
	{
		if ( argc <= 1 )
			error(2, 0, "option -c expects an operand");

		for ( int i = 2; i < argc; i++ )
		{
			char varname[sizeof(int) * 3];
			snprintf(varname, sizeof(varname), "%i", i - 2);
			setenv(varname, argv[i], 1);
		}

		const char* command = argv[1];
		size_t command_length = strlen(command);

		FILE* fp = fmemopen((void*) command, command_length, "r");
		if ( !fp )
			error(2, errno, "fmemopen");

		status = run(fp, "<command-line>", false, flag_e_exit_on_error,
		             &script_exited, status);

		fclose(fp);

		if ( script_exited || (status != 0 && flag_e_exit_on_error) )
			exit(status);

		if ( flag_s_stdin )
		{
			bool is_interactive = flag_i_interactive || isatty(fileno(stdin));
			status = run(stdin, "<stdin>", is_interactive, flag_e_exit_on_error,
			             &script_exited, status);
			if ( script_exited || (status != 0 && flag_e_exit_on_error) )
				exit(status);
		}
	}
	else if ( flag_s_stdin )
	{
		for ( int i = 1; i < argc; i++ )
		{
			char varname[sizeof(int) * 3];
			snprintf(varname, sizeof(varname), "%i", i - 1);
			setenv(varname, argv[i], 1);
		}

		bool is_interactive = flag_i_interactive || isatty(fileno(stdin));
		status = run(stdin, "<stdin>", is_interactive, flag_e_exit_on_error,
		             &script_exited, status);
		if ( script_exited || (status != 0 && flag_e_exit_on_error) )
			exit(status);
	}
	else if ( 2 <= argc )
	{
		for ( int i = 1; i < argc; i++ )
		{
			char varname[sizeof(int) * 3];
			snprintf(varname, sizeof(varname), "%i", i - 1);
			setenv(varname, argv[i], 1);
		}

		const char* path = argv[1];
		FILE* fp = fopen(path, "r");
		if ( !fp )
			error(127, errno, "%s", path);
		status = run(fp, path, false, flag_e_exit_on_error, &script_exited,
		             status);
		fclose(fp);
		if ( script_exited || (status != 0 && flag_e_exit_on_error) )
			exit(status);
	}
	else
	{
		bool is_interactive = flag_i_interactive || isatty(fileno(stdin));
		status = run(stdin, "<stdin>", is_interactive, flag_e_exit_on_error,
		             &script_exited, status);
		if ( script_exited || (status != 0 && flag_e_exit_on_error) )
			exit(status);
	}

	return 0;
}
