/*
 * Copyright (c) 2011, 2012, 2013, 2014, 2015 Jonas 'Sortie' Termansen.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * sh.c
 * Command language interpreter.
 */

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
#include <limits.h>
#include <locale.h>
#include <sched.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <wchar.h>
#include <wctype.h>

// Sortix libc doesn't have its own proper <limits.h> at this time.
#if defined(__sortix__)
#include <sortix/limits.h>
#endif

#include "editline.h"
#include "showline.h"
#include "util.h"

const char* builtin_commands[] =
{
	"cd",
	"exit",
	"unset",
	"clearenv",
	(const char*) NULL,
};

int status = 0;
bool foreground_shell;

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

void update_env(void)
{
	char str[3 * sizeof(size_t)];
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

void array_shrink_free(void*** array_ptr,
                       size_t* used_ptr,
                       size_t* length_ptr,
                       size_t new_used)
{
	void** array;
	memcpy(&array, array_ptr, sizeof(array)); // Strict aliasing.
	for ( size_t i = new_used; i < *used_ptr; i++ )
	{
		void* value;
		memcpy(&value, array + i, sizeof(value)); // Strict aliasing.
		free(value);
	}
	*used_ptr = new_used;
	(void) length_ptr;
}

char* token_finalize(const char* token)
{
	struct stringbuf buf;
	stringbuf_begin(&buf);
	bool escape = false;
	bool single_quote = false;
	bool double_quote = false;
	for ( size_t i = 0; token[i]; i++ )
	{
		if ( !escape && !single_quote && token[i] == '\\' )
		{
			escape = true;
		}
		else if ( !escape && !double_quote && token[i] == '\'' )
		{
			single_quote = !single_quote;
		}
		else if ( !escape && !single_quote && token[i] == '"' )
		{
			double_quote = !double_quote;
		}
		else if ( escape && token[i] == '\n' )
		{
			escape = false;
		}
		else
		{
			stringbuf_append_c(&buf, token[i]);
			escape = false;
		}
	}
	return stringbuf_finish(&buf);
}

bool is_identifier_char(char c)
{
	return ('a' <= c && c <= 'z') ||
	       ('A' <= c && c <= 'Z') ||
	       ('0' <= c && c <= '9') ||
	       c == '_';
}

char* token_expand_variables(const char* token)
{
	struct stringbuf buf;
	stringbuf_begin(&buf);
	bool escape = false;
	bool single_quote = false;
	bool double_quote = false;
	for ( size_t i = 0; token[i]; i++ )
	{
		if ( !escape && !single_quote && token[i] == '\\' )
		{
			stringbuf_append_c(&buf, '\\');
			escape = true;
		}
		else if ( !escape && !double_quote && token[i] == '\'' )
		{
			stringbuf_append_c(&buf, '\'');
			single_quote = !single_quote;
		}
		else if ( !escape && !single_quote && token[i] == '"' )
		{
			stringbuf_append_c(&buf, '"');
			double_quote = !double_quote;
		}
		else if ( !escape && !single_quote && token[i] == '$' && token[i + 1] )
		{
			i++;
			const char* value;
			if ( token[i] == '{' )
			{
				i++;
				size_t length = 0;
				while ( token[i + length] && token[i + length] != '}' )
					length++;
				char* variable = strndup(token + i, length);
				if ( !variable )
					return free(buf.string), (char*) NULL;
				value = getenv(variable);
				free(variable);
				i += length;
				if ( token[i] == '}' )
					i++;
				i--;
			}
			else if ( is_identifier_char(token[i]) )
			{
				size_t length = 1;
				while ( is_identifier_char(token[i + length]) )
					length++;
				char* variable = strndup(token + i, length);
				if ( !variable )
					return free(buf.string), (char*) NULL;
				value = getenv(variable);
				free(variable);
				i += length - 1;
			}
			else
			{
				char variable[2] = { token[i], '\0' };
				value = getenv(variable);
			}
			for ( size_t n = 0; value && value[n]; n++ )
			{
				if ( double_quote && might_need_shell_quote(value[i]) )
					stringbuf_append_c(&buf, '\\');
				stringbuf_append_c(&buf, value[n]);
			}
		}
		else
		{
			stringbuf_append_c(&buf, token[i]);
			escape = false;
		}
	}
	return stringbuf_finish(&buf);
}

bool token_split(void*** out,
                 size_t* out_used,
                 size_t* out_length,
                 const char* token)
{
	size_t old_used = *out_used;
	size_t index = 0;
	while ( true )
	{
		while ( token[index] && isspace((unsigned char) token[index]) )
			index++;
		if ( !token[index] )
			break;
		struct stringbuf buf;
		stringbuf_begin(&buf);
		bool escape = false;
		bool single_quote = false;
		bool double_quote = false;
		for ( ; token[index]; index++ )
		{
			if ( !escape && !single_quote && token[index] == '\\' )
			{
				stringbuf_append_c(&buf, '\\');
				escape = true;
			}
			else if ( !escape && !double_quote && token[index] == '\'' )
			{
				stringbuf_append_c(&buf, '\'');
				single_quote = !single_quote;
			}
			else if ( !escape && !single_quote && token[index] == '"' )
			{
				stringbuf_append_c(&buf, '"');
				double_quote = !double_quote;
			}
			else if ( !(escape || single_quote || double_quote) &&
			          isspace((unsigned char) token[index]) )
			{
				break;
			}
			else if ( escape && token[index] == '\n' )
			{
				escape = false;
			}
			else
			{
				stringbuf_append_c(&buf, token[index]);
				escape = false;
			}
		}
		char* value = stringbuf_finish(&buf);
		if ( !value )
		{
			array_shrink_free(out, out_used, out_length, old_used);
			return false;
		}
		if ( !array_add(out, out_used, out_length, value) )
		{
			array_shrink_free(out, out_used, out_length, old_used);
			return false;
		}
	}
	return true;
}

bool token_expand_variables_split(void*** out,
                                  size_t* out_used,
                                  size_t* out_length,
                                  const char* token)
{
	char* expanded = token_expand_variables(token);
	if ( !expanded )
		return false;
	bool result = token_split(out, out_used, out_length, expanded);
	free(expanded);
	return result;
}

static int strcoll_indirect(const void* a_ptr, const void* b_ptr)
{
	const char* a = *(const char* const*) a_ptr;
	const char* b = *(const char* const*) b_ptr;
	return strcoll(a, b);
}

bool token_expand_wildcards(void*** out,
                            size_t* out_used,
                            size_t* out_length,
                            const char* token)
{
	size_t old_used = *out_used;

	size_t index = 0;
	size_t num_escaped_wildcards = 0; // We don't properly support them yet.

	struct stringbuf buf;
	stringbuf_begin(&buf);

	bool escape = false;
	bool single_quote = false;
	bool double_quote = false;
	for ( ; token[index]; index++ )
	{
		if ( !escape && !single_quote && token[index] == '\\' )
		{
			escape = true;
		}
		else if ( !escape && !double_quote && token[index] == '\'' )
		{
			single_quote = !single_quote;
		}
		else if ( !escape && !single_quote && token[index] == '"' )
		{
			double_quote = !double_quote;
		}
		else if ( !(escape || single_quote || double_quote) &&
		          token[index] == '*' )
		{
			break;
		}
		else
		{
			if ( token[index] == '*' )
				num_escaped_wildcards++;
			stringbuf_append_c(&buf, token[index]);
			escape = false;
		}
	}

	if ( token[index] != '*' || num_escaped_wildcards )
	{
		char* value;
		free(stringbuf_finish(&buf));
	just_return_input:
		value = strdup(token);
		if ( !value )
			return false;
		if ( !array_add(out, out_used, out_length, value) )
			return free(value), false;
		return true;
	}

	char* before = stringbuf_finish(&buf);
	if ( !before )
		return false;
	stringbuf_begin(&buf);

	index++;

	for ( ; token[index]; index++ )
	{
		if ( !escape && !single_quote && token[index] == '\\' )
		{
			escape = true;
		}
		else if ( !escape && !double_quote && token[index] == '\'' )
		{
			single_quote = !single_quote;
		}
		else if ( !escape && !single_quote && token[index] == '"' )
		{
			double_quote = !double_quote;
		}
		else if ( !(escape || single_quote || double_quote) &&
		          token[index] == '*' )
		{
			break;
		}
		else
		{
			if ( token[index] == '*' )
				num_escaped_wildcards++;
			stringbuf_append_c(&buf, token[index]);
			escape = false;
		}
	}

	if ( token[index] == '*' )
	{
		// TODO: We don't support double use of wildcards yet.
		free(stringbuf_finish(&buf));
		free(before);
		goto just_return_input;
	}

	char* after = stringbuf_finish(&buf);
	if ( !after )
		return free(before), false;

	char* pattern;
	if ( asprintf(&pattern, "%s*%s", before, after) < 0 )
		return free(after), free(before), false;
	free(after);
	free(before);

	size_t wildcard_pos = strcspn(pattern, "*");
	bool found_slash = false;
	size_t last_slash = 0;
	for ( size_t n = 0; n < wildcard_pos; n++ )
		if ( pattern[n] == '/' )
			last_slash = n, found_slash = true;
	size_t match_from = found_slash ? last_slash + 1 : 0;

	size_t pattern_prefix = 0;
	DIR* dir = NULL;
	if ( !found_slash )
	{
		if ( !(dir = opendir(".")) )
		{
			free(pattern);
			goto just_return_input;
		}
	}
	else
	{
		char* dirpath = strdup(pattern);
		if ( !dirpath )
		{
			free(pattern);
			goto just_return_input;
		}
		dirpath[last_slash] = '\0';
		pattern_prefix = last_slash + 1;
		dir = opendir(dirpath);
		free(dirpath);
		if ( !dir )
		{
			free(pattern);
			goto just_return_input;
		}
	}
	size_t num_inserted = 0;
	struct dirent* entry;
	while ( (entry = readdir(dir)) )
	{
		if ( !matches_simple_pattern(entry->d_name, pattern + match_from) )
			continue;
		stringbuf_begin(&buf);
		for ( size_t i = 0; i < pattern_prefix; i++ )
		{
			if ( pattern[i] == '\n' )
			{
				stringbuf_append_c(&buf, '\'');
				stringbuf_append_c(&buf, '\n');
				stringbuf_append_c(&buf, '\'');
			}
			else
			{
				if ( might_need_shell_quote(pattern[i]) )
					stringbuf_append_c(&buf, '\\');
				stringbuf_append_c(&buf, pattern[i]);
			}
		}
		for ( size_t i = 0; entry->d_name[i]; i++ )
		{
			if ( entry->d_name[i] == '\n' )
			{
				stringbuf_append_c(&buf, '\'');
				stringbuf_append_c(&buf, '\n');
				stringbuf_append_c(&buf, '\'');
			}
			else
			{
				if ( might_need_shell_quote(entry->d_name[i]) )
					stringbuf_append_c(&buf, '\\');
				stringbuf_append_c(&buf, entry->d_name[i]);
			}
		}
		char* name = stringbuf_finish(&buf);
		if ( !name )
		{
			free(pattern);
			closedir(dir);
			array_shrink_free(out, out_used, out_length, old_used);
			return false;
		}
		if ( !array_add(out, out_used, out_length, name) )
		{
			free(name);
			free(pattern);
			closedir(dir);
			array_shrink_free(out, out_used, out_length, old_used);
			return false;
		}
		num_inserted++;
	}
	closedir(dir);
	free(pattern);
	if ( num_inserted == 0 )
		goto just_return_input;
	char** out_tokens;
	memcpy(&out_tokens, out, sizeof(out_tokens));
	char** sort_from = out_tokens + old_used;
	size_t sort_count = *out_used - old_used;
	qsort(sort_from, sort_count, sizeof(char*), strcoll_indirect);
	return true;
}

enum sh_tokenize_result
{
	SH_TOKENIZE_RESULT_OK,
	SH_TOKENIZE_RESULT_PARTIAL,
	SH_TOKENIZE_RESULT_INVALID,
	SH_TOKENIZE_RESULT_ERROR,
};

bool can_continue_operator(const char* op, char c)
{
	if ( !op )
		return false;
	if ( op[0] == '<' && op[1] == '<' && op[2] == '\0' )
		return c == '-';
	if ( op[0] == '|' && op[1] == '\0' )
		return c == '|';
	if ( op[0] == '&' && op[1] == '\0' )
		return c == '&';
	if ( op[0] == ';' && op[1] == '\0' )
		return c == ';';
	if ( op[0] == '<' && op[1] == '\0' )
		return c == '<' || c == '&' || c == '>';
	if ( op[0] == '>' && op[1] == '\0' )
		return c == '>' || c == '&' || c == '|';
	if ( op[0] == '\0' )
		return c == '|' || c == '&' || c == ';' || c == '>' || c == '<' ||
		       c == '(' || c == ')';
	return false;
}

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

		struct stringbuf buf;
		stringbuf_begin(&buf);

		bool escaped = false;
		bool single_quote = false;
		bool double_quote = false;
		bool stop = false;
		bool making_operator = false;
		while ( true )
		{
			if ( command[command_index] == '\0' )
			{
				if ( escaped || single_quote || double_quote )
					result = SH_TOKENIZE_RESULT_PARTIAL;
				stop = true;
				break;
			}
			else if ( making_operator )
			{
				if ( can_continue_operator(buf.string, command[command_index]) )
				{
					stringbuf_append_c(&buf, command[command_index]);
					command_index++;
				}
				else
				{
					break;
				}
			}
			else if ( buf.string && buf.length == 0 &&
			          can_continue_operator("", command[command_index]) )
			{
				stringbuf_append_c(&buf, command[command_index]);
				making_operator = true;
				command_index++;
			}
			else if ( !escaped && !single_quote && command[command_index] == '\\' )
			{
				stringbuf_append_c(&buf, '\\');
				escaped = true;
				command_index++;
			}
			else if ( !escaped && !double_quote && command[command_index] == '\'' )
			{
				stringbuf_append_c(&buf, '\'');
				single_quote = !single_quote;
				command_index++;
			}
			else if ( !escaped && !single_quote && command[command_index] == '"' )
			{
				stringbuf_append_c(&buf, '"');
				double_quote = !double_quote;
				command_index++;
			}
			else if ( !(escaped || single_quote || double_quote) &&
			          (isspace((unsigned char) command[command_index]) ||
			           can_continue_operator("", command[command_index])) )
			{
				break;
			}
			else if ( escaped && command[command_index] == '\n' )
			{
				if ( buf.string && buf.length && buf.string[buf.length - 1] == '\\' )
					buf.string[--buf.length] = '\0';
				command_index++;
				escaped = false;
			}
			else
			{
				stringbuf_append_c(&buf, command[command_index]);
				command_index++;
				escaped = false;
			}
		}

		char* token = stringbuf_finish(&buf);
		if ( !token )
		{
			result = SH_TOKENIZE_RESULT_ERROR;
			break;
		}

		if ( !array_add((void***) &tokens,
		                &tokens_used,
		                &tokens_length,
		                token) )
		{
			free(token);
			result = SH_TOKENIZE_RESULT_ERROR;
			break;
		}

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

bool is_variable_assignment_token(const char* token)
{
	size_t i = 0;
	while ( is_identifier_char(token[i]) )
		i++;
	return i != 0 && token[i] == '=';
}

struct execute_result
{
	pid_t pid;
	int internal_status;
	bool failure;
	bool critical;
	bool internal;
	bool exited;
};

struct execute_result execute(char** tokens,
                              size_t tokens_count,
                              bool interactive,
                              int pipein,
                              int pipeout,
                              pid_t pgid)
{
	char** varsv;
	size_t varsc;
	size_t varsv_allocated;
	char** expandv;
	size_t expandc;
	size_t expandv_allocated;
	char** argv;
	size_t argc;
	size_t argv_allocated;
	bool internal;
	bool failure = false;
	bool critical = false;
	bool do_exit = false;
	bool set_pipein = false;
	bool set_pipeout = false;
	bool had_not_varassign = false;
	pid_t childpid;

	update_env();
	char statusstr[sizeof(int) * 3];
	snprintf(statusstr, sizeof(statusstr), "%i", status);
	setenv("?", statusstr, 1);

	varsv = NULL;
	varsc = 0;
	varsv_allocated = 0;

	expandv = NULL;
	expandc = 0;
	expandv_allocated = 0;

	for ( size_t i = 0; !failure && i < tokens_count; i++ )
	{
		if ( !had_not_varassign && is_variable_assignment_token(tokens[i]))
		{
			char* value = token_expand_variables(tokens[i]);
			if ( !value )
			{
				error(0, errno, "variable expansion");
				failure = true;
				critical = true;
				break;
			}
			if ( !array_add((void***) &varsv,
			                &varsc,
			                &varsv_allocated,
			                value) )
			{
				free(value);
				error(0, errno, "variable expansion");
				failure = true;
				critical = true;
				break;
			}
		}
		else
		{
			had_not_varassign = true;
			if ( !token_expand_variables_split((void***) &expandv,
				                               &expandc,
				                               &expandv_allocated,
				                               tokens[i]) )
			{
				error(0, errno, "variable expansion");
				failure = true;
				critical = true;
				break;
			}
		}
	}

	argv = NULL;
	argc = 0;
	argv_allocated = 0;

	for ( size_t i = 0; !failure && i < expandc; i++ )
	{
		if ( !strcmp(expandv[i], "<") ||
		     !strcmp(expandv[i], ">") ||
		   /*!strcmp(expandv[i], "<<") ||*/
		     !strcmp(expandv[i], ">>") )
		{
			const char* type = expandv[i++];

			if ( i == expandc )
			{
				error(0, errno, "%s: expected argument", type);
				failure = true;
				critical = true;
				break;
			}

			char** targets = NULL;
			size_t targets_used = 0;
			size_t targets_length = 0;

			if ( !token_expand_wildcards((void***) &targets,
				                         &targets_used,
				                         &targets_length,
				                         expandv[i]) )
			{
				error(0, errno, "wildcard expansion");
				failure = true;
				critical = true;
				break;
			}

			if ( targets_used != 1 )
			{
				error(0, 0, "%s: ambiguous redirect: %s", type, expandv[i]);
				for ( size_t i = 0; i < targets_used; i++ )
					free(targets[i]);
				free(targets);
				failure = true;
				break;
			}

			char* target = token_finalize(targets[0]);
			free(targets[0]);
			free(targets);
			if ( !target )
			{
				error(0, errno, "token finalization");
				failure = true;
				break;
			}

			int fd = -1;
			if ( !strcmp(type, "<") )
				fd = open(target, O_RDONLY | O_CLOEXEC);
			else if ( !strcmp(type, ">") )
				fd = open(target, O_WRONLY | O_CREATE | O_TRUNC | O_CLOEXEC, 0666);
			else if ( !strcmp(type, ">>") )
				fd = open(target, O_WRONLY | O_CREATE | O_APPEND | O_CLOEXEC, 0666);

			if ( fd < 0 )
			{
				error(0, errno, "%s", target);
				free(target);
				failure = true;
				break;
			}

			if ( !strcmp(type, "<") )
			{
				pipein = fd;
				set_pipein = true;
			}
			else if ( !strcmp(type, ">") || !strcmp(type, ">>") )
			{
				pipeout = fd;
				set_pipeout = true;
			}

			free(target);
		}
		else
		{
			if ( !token_expand_wildcards((void***) &argv,
				                         &argc,
				                         &argv_allocated,
				                         expandv[i]) )
			{
				error(0, errno, "wildcard expansion");
				failure = true;
				critical = true;
				break;
			}
		}
	}

	for ( size_t i = 0; i < expandc; i++ )
		free(expandv[i]);
	free(expandv);

	if ( !array_add((void***) &argv, &argc, &argv_allocated, (char*) NULL) )
	{
		failure = true;
		critical = true;
	}
	else
	{
		argc--; // Don't count the NULL.
	}

	for ( size_t i = 0; i < varsc; i++ )
	{
		char* var = token_finalize(varsv[i]);
		if ( !var )
		{
			error(0, errno, "token finalization");
			failure = true;
			break;
		}
		free(varsv[i]);
		varsv[i] = var;
	}

	for ( size_t i = 0; i < argc; i++ )
	{
		char* arg = token_finalize(argv[i]);
		if ( !arg )
		{
			error(0, errno, "token finalization");
			failure = true;
			break;
		}
		free(argv[i]);
		argv[i] = arg;
	}

	// TODO: Builtins should be run in a child process if & or |.
	int internal_status = status;
	if ( failure )
	{
		internal = true;
		internal_status = 1;
	}
	else if ( argc == 0 )
	{
		internal = true;
		for ( size_t i = 0; i < varsc; i++ )
		{
			char* key = varsv[i];
			char* eq = strchr(key, '=');
			if ( !eq )
				continue;
			*eq = '\0';
			char* value = eq + 1;
			int ret = setenv(key, value, 1);
			*eq = '=';
			if ( ret < 0 )
			{
				error(0, errno, "setenv");
				internal_status = 1;
			}
		}
	}
	else if ( strcmp(argv[0], "cd") == 0 )
	{
		internal = true;
		const char* newdir = argv[1];
		if ( !newdir )
			newdir = getenv_safe_def("HOME", "/");
		internal_status = 0;
		if ( perform_chdir(newdir) < 0 )
		{
			error(0, errno, "cd: %s", newdir);
			internal_status = 1;
		}
	}
	else if ( strcmp(argv[0], "exit") == 0 )
	{
		internal = true;
		int exitcode = argv[1] ? atoi(argv[1]) : 0;
		exitcode = (unsigned char) exitcode;
		do_exit = true;
		internal_status = exitcode;
	}
	else if ( strcmp(argv[0], "export") == 0 )
	{
		internal = true;
		internal_status = 0;
		if ( argv[1] )
		{
			size_t eqpos = strcspn(argv[1], "=");
			if ( argv[1][eqpos] == '=' )
			{
				char* name = strndup(argv[1], eqpos);
				if ( name )
				{
					if ( setenv(name, argv[1] + eqpos + 1, 1) < 0 )
					{
						error(0, errno, "export: setenv");
						internal_status = 1;
					}
				}
				else
				{
					error(0, errno, "export: malloc");
					internal_status = 1;
				}
			}
		}
		else
		{
			for ( size_t i = 0; environ[i]; i++ )
			{
				const char* envvar = environ[i];
				printf("export ");
				while ( *envvar && *envvar != '=' )
					putchar((unsigned char) *envvar++);
				if ( *envvar == '=' )
					putchar((unsigned char) *envvar++);
				putchar('\'');
				while ( *envvar )
				{
					if ( *envvar == '\'' )
					{
						putchar('\'');
						putchar('\\');
						putchar((unsigned char) *envvar++);
						putchar('\'');
					}
					else
					{
						putchar((unsigned char) *envvar++);
					}
				}
				putchar('\'');
				putchar('\n');
			}
		}

	}
	else if ( strcmp(argv[0], "unset") == 0 )
	{
		internal = true;
		unsetenv(argv[1] ? argv[1] : "");
		internal_status = 0;
	}
	else if ( strcmp(argv[0], "clearenv") == 0 )
	{
		internal = true;
		clearenv();
		internal_status = 0;
	}
	else
	{
		internal = false;
	}

	if ( (childpid = internal ? getpid() : fork()) < 0 )
	{
		error(0, errno, "fork");
		internal_status = 1;
		failure = true;
		internal = true;
		childpid = getpid();
	}

	if ( childpid )
	{
		if ( set_pipein )
			close(pipein);

		if ( set_pipeout )
			close(pipeout);

		for ( size_t i = 0; i < varsc; i++ )
			free(varsv[i]);
		free(varsv);

		for ( size_t i = 0; i < argc; i++ )
			free(argv[i]);
		free(argv);

		if ( internal )
		{
			struct execute_result result;
			memset(&result, 0, sizeof(result));
			result.internal_status = internal_status;
			result.failure = failure;
			result.critical = critical;
			result.internal = true;
			result.exited = do_exit;
			return result;
		}

		setpgid(childpid, pgid != -1 ? pgid : childpid);
		// TODO: This is an inefficient manner to avoid a race condition where
		//       a pipeline foo | bar is running in its own process group and
		//       foo is the process group leader that sets itself as the
		//       foreground process group, but bar dies prior to foo's tcsetpgrp
		//       call, because then the shell would run tcsetpgrp to take back
		//       control, and only then would foo do its tcsetpgrp call.
		while ( foreground_shell && pgid == -1 && tcgetpgrp(0) != childpid )
			sched_yield();

		struct execute_result result;
		memset(&result, 0, sizeof(result));
		result.pid = childpid;
		result.internal = false;
		return result;
	}

	setpgid(0, pgid != -1 ? pgid : 0);
	if ( foreground_shell && pgid == -1 )
	{
		sigset_t oldset, sigttou;
		sigemptyset(&sigttou);
		sigaddset(&sigttou, SIGTTOU);
		sigprocmask(SIG_BLOCK, &sigttou, &oldset);
		tcsetpgrp(0, getpgid(0));
		sigprocmask(SIG_SETMASK, &oldset, NULL);
	}

	if ( pipein != 0 )
		dup2(pipein, 0);

	if ( pipeout != 1 )
		dup2(pipeout, 1);

	for ( size_t i = 0; i < varsc; i++ )
	{
		char* key = varsv[i];
		char* eq = strchr(key, '=');
		if ( !eq )
			continue;
		*eq = '\0';
		char* value = eq + 1;
		int ret = setenv(key, value, 1);
		*eq = '=';
		if ( ret < 0 )
		{
			error(0, errno, "setenv");
			status = 1;
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

	__builtin_unreachable();
}

int run_tokens(char** tokens,
               size_t tokens_count,
               bool interactive,
               bool exit_on_error,
               bool* script_exited)
{
	size_t cmdnext = 0;
	size_t cmdstart;
	size_t cmdend;
	int pipein = 0;
	int pipeout = 1;
	int pipeinnext = 0;
	const char* execmode;
	pid_t pgid = -1;
	bool short_circuited_and = false;
	bool short_circuited_or = false;

	// Collect any pending zombie processes.
	while ( 0 < waitpid(-1, NULL, WNOHANG) );

readcmd:
	cmdstart = cmdnext;

	if ( cmdstart == tokens_count )
		return status;

	for ( cmdend = cmdstart; cmdend < tokens_count; cmdend++ )
	{
		const char* token = tokens[cmdend];
		if ( strcmp(token, ";") == 0 ||
		     strcmp(token, "&") == 0 ||
		     strcmp(token, "&&") == 0 ||
		     strcmp(token, "|") == 0 ||
		     strcmp(token, "||") == 0 ||
		     false )
			break;
	}

	if ( cmdend < tokens_count )
	{
		execmode = tokens[cmdend];
		cmdnext = cmdend + 1;
	}
	else
	{
		execmode = ";";
		cmdnext = cmdend;
	}

	if ( short_circuited_or )
	{
		if ( !strcmp(execmode, ";") ||
		     !strcmp(execmode, "&") )
		{
			short_circuited_and = false;
			short_circuited_or = false;
		}
		goto readcmd;
	}

	if ( short_circuited_and )
	{
		if ( !strcmp(execmode, ";") ||
		     !strcmp(execmode, "&") ||
		     !strcmp(execmode, "||") )
			short_circuited_and = false;
		goto readcmd;
	}

	if ( strcmp(execmode, "|") == 0 )
	{
		int pipes[2];
		if ( pipe2(pipes, O_CLOEXEC) < 0 )
		{
			error(0, errno, "pipe");
			if ( !interactive || exit_on_error )
				*script_exited = true;
			return status = 1;
		}
		else
		{
			if ( pipeout != 1 )
				close(pipeout);
			pipeout = pipes[1];
			if ( pipeinnext != 0 )
				close(pipeinnext);
			pipeinnext = pipes[0];
		}
	}

	struct execute_result result =
		execute(tokens + cmdstart,
		        cmdend - cmdstart,
		        interactive,
		        pipein,
		        pipeout,
		        pgid);

	if ( !result.internal && pgid == -1 )
		pgid = result.pid;

	if ( pipein != 0 )
	{
		close(pipein);
		pipein = 0;
	}

	if ( pipeout != 1 )
	{
		close(pipeout);
		pipeout = 1;
	}

	if ( pipeinnext != 0 )
	{
		pipein = pipeinnext;
		pipeinnext = 0;
	}

	if ( result.critical )
	{
		if ( !interactive || exit_on_error )
			*script_exited = true;
		return status = result.internal_status;
	}

	if ( result.exited )
	{
		*script_exited = true;
		return status = result.internal_status;
	}

	if ( strcmp(execmode, "&") == 0 )
	{
		// TODO: We probably shouldn't have made the progress group foreground
		//       as that may have side effects if a process checks for this
		//       behavior and then unexpectedly the shell takes back support
		//       without the usual ^Z mechanism.
		sigset_t oldset, sigttou;
		sigemptyset(&sigttou);
		sigaddset(&sigttou, SIGTTOU);
		sigprocmask(SIG_BLOCK, &sigttou, &oldset);
		tcsetpgrp(0, getpgid(0));
		sigprocmask(SIG_SETMASK, &oldset, NULL);
		pgid = -1;
		status = 0;
		goto readcmd;
	}

	if ( strcmp(execmode, "|") == 0 )
		goto readcmd;

	if ( result.internal )
	{
		status = result.internal_status;
	}
	else
	{
		int exitstatus;
		if ( waitpid(result.pid, &exitstatus, 0) < 0 )
		{
			error(0, errno, "waitpid");
			if ( !interactive || exit_on_error )
				*script_exited = true;
			status = 1;
			return status;
		}
		sigset_t oldset, sigttou;
		sigemptyset(&sigttou);
		sigaddset(&sigttou, SIGTTOU);
		sigprocmask(SIG_BLOCK, &sigttou, &oldset);
		tcsetpgrp(0, getpgid(0));
		sigprocmask(SIG_SETMASK, &oldset, NULL);
		if ( WIFSIGNALED(exitstatus) && WTERMSIG(exitstatus) == SIGINT )
			printf("^C\n");
		else if ( WIFSIGNALED(exitstatus) && WTERMSIG(exitstatus) != SIGPIPE )
			printf("%s\n", strsignal(WTERMSIG(exitstatus)));
		status = WEXITSTATUS(exitstatus);
	}

	pgid = -1;

	if ( !strcmp(execmode, "&&") )
	{
		if ( status != 0 )
			short_circuited_and = true;
	}
	else if ( !strcmp(execmode, "||") )
	{
		if ( status == 0 )
			short_circuited_or = true;
	}
	else if ( exit_on_error && status != 0 )
	{
		*script_exited = true;
		return status;
	}

	goto readcmd;
}

int run_command(char* command,
                bool interactive,
                bool exit_on_error,
                bool* script_exited)
{
	int result;

	char** tokens = NULL;
	size_t tokens_used = 0;
	size_t tokens_length = 0;

	enum sh_tokenize_result tokenize_result =
		sh_tokenize(command, &tokens, &tokens_used, &tokens_length);

	if ( tokenize_result == SH_TOKENIZE_RESULT_OK )
	{
		result = run_tokens(tokens, tokens_used, interactive, exit_on_error,
		                    script_exited);
	}
	else
	{
		if ( !interactive )
			*script_exited = true;
		result = 255;
	}

	for ( size_t i = 0; i < tokens_used; i++ )
		free(tokens[i]);
	free(tokens);

	return result;
}

bool does_line_editing_need_another_line(void* ctx, const char* line)
{
	(void) ctx;
	return !is_shell_input_ready(line);
}

bool is_outermost_shell(void)
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
                   void* ctx,
                   const char* partial,
                   size_t complete_at)
{
	(void) ctx;

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

	char** completions = NULL;
	size_t completions_count = 0;
	size_t completions_length = 0;

	if ( complete_type == COMPLETE_TYPE_PROGRAM ) do
	{
		for ( size_t i = 0; builtin_commands[i]; i++ )
		{
			const char* builtin = builtin_commands[i];
			if ( strncmp(builtin, partial + complete_at - used_before, used_before) != 0 )
				continue;
			// TODO: Add allocation check!
			array_add((void***) &completions,
			          &completions_count,
			          &completions_length,
			          strdup(builtin + used_before));
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
			DIR* dir;
			if ( (dir = opendir(component)) )
			{
				struct dirent* entry;
				while ( (entry = readdir(dir)) )
				{
					if ( strncmp(entry->d_name, partial + complete_at - used_before, used_before) != 0 )
						continue;
					if ( used_before == 0 &&  entry->d_name[0] == '.' )
						continue;
					// TODO: Add allocation check!
					array_add((void***) &completions,
					          &completions_count,
					          &completions_length,
					          strdup(entry->d_name + used_before));
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
		struct dirent* entry;
		while ( (entry = readdir(dir)) )
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
			// TODO: Add allocation check!
			array_add((void***) &completions,
			          &completions_count,
			          &completions_length,
			          completion);
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
			// TODO: Add allocation check!
			array_add((void***) &completions,
			          &completions_count,
			          &completions_length,
			          strndup(rest, equal_offset));
		}
	} while ( false );

	*used_before_ptr = used_before;
	*used_after_ptr = used_after;

	return *completions_ptr = completions, completions_count;
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
	char hostname[HOST_NAME_MAX + 1];
	if ( gethostname(hostname, sizeof(hostname)) < 0 )
		strlcpy(hostname, "(none)", sizeof(hostname));
	const char* print_hostname = hostname;
	const char* print_dir = current_dir ? current_dir : "?";
	const char* home_dir = getenv_safe("HOME");

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
	if ( !isatty(fileno(fp)) || !foreground_shell )
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

static void version(FILE* fp, const char* argv0)
{
	fprintf(fp, "%s (Sortix) %s\n", argv0, VERSIONSTR);
}

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "");

	foreground_shell = isatty(0) && tcgetpgrp(0) == getpgid(0);

	// TODO: Canonicalize argv[0] if it contains a slash and isn't absolute?

	const char* env_pwd;
	if ( (env_pwd = getenv("PWD")) )
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
			break;  // Intentionally not continue and note '+' support.
		argv[i] = NULL;
		if ( !strcmp(arg, "--") )
			break;
		if ( arg[0] == '+' )
		{
			char c;
			while ( (c = *++arg) ) switch ( c )
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
			char c;
			while ( (c = *++arg) ) switch ( c )
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
		else if ( shlvl < LONG_MAX )
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
