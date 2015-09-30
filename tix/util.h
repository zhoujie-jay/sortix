/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2015.

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

    util.h
    Shared Utility functions for Tix.

*******************************************************************************/

#ifndef UTIL_H
#define UTIL_H

// TODO: After releasing Sortix 1.0, remove this ifdef and default it to the
//       latest generation.
#if defined(__sortix__)
#define DEFAULT_GENERATION "2"
#else
#define DEFAULT_GENERATION "1"
#endif

bool does_path_contain_dotdot(const char* path)
{
	size_t index = 0;
	while ( path[index] )
	{
		if ( path[index] == '.' && path[index+1] == '.' )
		{
			index += 2;
			if ( !path[index] || path[index] == '/' )
				return true;
		}
		while ( path[index] && path[index] != '/' )
			index++;
		while ( path[index] == '/' )
			index++;
	}
	return false;
}

bool parse_boolean(const char* str)
{
	return strcmp(str, "true") == 0;
}

char* strdup_null(const char* src)
{
	return src ? strdup(src) : NULL;
}

char* strdup_null_if_content(const char* src)
{
	if ( src && !src[0] )
		return NULL;
	return strdup_null(src);
}

const char* non_modify_basename(const char* path)
{
	const char* last_slash = (char*) strrchr((char*) path, '/');
	if ( !last_slash )
		return path;
	return last_slash + 1;
}

typedef struct
{
	char** strings;
	size_t length;
	size_t capacity;
} string_array_t;

string_array_t string_array_make()
{
	string_array_t sa;
	sa.strings = NULL;
	sa.length = sa.capacity = 0;
	return sa;
}

void string_array_reset(string_array_t* sa)
{
	for ( size_t i = 0; i < sa->length; i++ )
		free(sa->strings[i]);
	free(sa->strings);
	*sa = string_array_make();
}

bool string_array_append(string_array_t* sa, const char* str)
{
	if ( sa->length == sa->capacity )
	{
		size_t new_capacity = sa->capacity ? sa->capacity * 2 : 8;
		size_t new_size = sizeof(char*) * new_capacity;
		char** new_strings = (char**) realloc(sa->strings, new_size);
		if ( !new_strings )
			return false;
		sa->strings = new_strings;
		sa->capacity = new_capacity;
	}
	char* copy = strdup_null(str);
	if ( str && !copy )
		return false;
	sa->strings[sa->length++] = copy;
	return true;
}

bool string_array_append_token_string(string_array_t* sa, const char* str)
{
	while ( *str )
	{
		if ( isspace((unsigned char) *str) )
		{
			str++;
			continue;
		}

		size_t input_length = 0;
		size_t output_length = 0;
		bool quoted = false;
		bool escaped = false;
		while ( str[input_length] &&
		        (escaped || quoted || !isspace((unsigned char) str[input_length])) )
		{
			if ( !escaped && str[input_length] == '\\' )
				escaped = true;
			else if ( !escaped && str[input_length] == '"' )
				quoted = !quoted;
			else
				escaped = false, output_length++;
			input_length++;
		}

		if ( quoted || escaped )
			return false;

		char* output = (char*) malloc(sizeof(char) * (output_length+1));
		if ( !output )
			return false;

		input_length = 0;
		output_length = 0;
		quoted = false;
		escaped = false;
		while ( str[input_length] &&
		        (escaped || quoted || !isspace((unsigned char) str[input_length])) )
		{
			if ( !escaped && str[input_length] == '\\' )
				escaped = true;
			else if ( !escaped && str[input_length] == '"' )
				quoted = !quoted;
			else if ( escaped && str[input_length] == 'a' )
				escaped = false, output[output_length++] = '\a';
			else if ( escaped && str[input_length] == 'b' )
				escaped = false, output[output_length++] = '\b';
			else if ( escaped && str[input_length] == 'e' )
				escaped = false, output[output_length++] = '\e';
			else if ( escaped && str[input_length] == 'f' )
				escaped = false, output[output_length++] = '\f';
			else if ( escaped && str[input_length] == 'n' )
				escaped = false, output[output_length++] = '\n';
			else if ( escaped && str[input_length] == 'r' )
				escaped = false, output[output_length++] = '\r';
			else if ( escaped && str[input_length] == 't' )
				escaped = false, output[output_length++] = '\t';
			else if ( escaped && str[input_length] == 'v' )
				escaped = false, output[output_length++] = '\v';
			else
				escaped = false, output[output_length++] = str[input_length];
			input_length++;
		}

		output[output_length] = '\0';

		if ( !string_array_append(sa, output) )
		{
			free(output);
			return false;
		}

		free(output);

		str += input_length;
	}

	return true;
}

bool is_token_string_special_character(char c)
{
	return isspace((unsigned char) c) || c == '"' || c == '\\';
}

char* token_string_of_string_array(const string_array_t* sa)
{
	size_t result_length = 0;

	for ( size_t i = 0; i < sa->length; i++ )
	{
		if ( i )
			result_length++;

		for ( size_t n = 0; sa->strings[i][n]; n++ )
		{
			if ( is_token_string_special_character(sa->strings[i][n]) )
				result_length++;
			result_length++;
		}
	}

	char* result = (char*) malloc(sizeof(char) * (result_length + 1));
	if ( !result )
		return NULL;

	result_length = 0;

	for ( size_t i = 0; i < sa->length; i++ )
	{
		if ( i )
			result[result_length++] = ' ';

		for ( size_t n = 0; sa->strings[i][n]; n++ )
		{
			if ( is_token_string_special_character(sa->strings[i][n]) )
				result[result_length++] = '\\';
			result[result_length++] = sa->strings[i][n];
		}
	}

	result[result_length] = '\0';

	return result;
}

void string_array_append_file(string_array_t* sa, FILE* fp)
{
	char* entry = NULL;
	size_t entry_size = 0;
	ssize_t entry_length;
	while ( 0 < (entry_length = getline(&entry, &entry_size, fp)) )
	{
		if ( entry_length && entry[entry_length-1] == '\n' )
			entry[entry_length-1] = '\0';
		string_array_append(sa, entry);
	}
	free(entry);
}

bool string_array_append_file_path(string_array_t* sa, const char* path)
{
	FILE* fp = fopen(path, "r");
	if ( !fp )
		return false;
	string_array_append_file(sa, fp);
	fclose(fp);
	return true;
}

size_t string_array_find(string_array_t* sa, const char* str)
{
	for ( size_t i = 0; i < sa->length; i++ )
		if ( !strcmp(sa->strings[i], str) )
			return i;
	return SIZE_MAX;
}

bool string_array_contains(string_array_t* sa, const char* str)
{
	return string_array_find(sa, str) != SIZE_MAX;
}

size_t dictionary_lookup_index(string_array_t* sa, const char* key)
{
	size_t keylen = strlen(key);
	for ( size_t i = 0; i < sa->length; i++ )
	{
		const char* entry = sa->strings[i];
		if ( strncmp(key, entry, keylen) != 0 )
			continue;
		if ( entry[keylen] != '=' )
			continue;
		return i;
	}
	return SIZE_MAX;
}

const char* dictionary_get_entry(string_array_t* sa, const char* key)
{
	size_t index = dictionary_lookup_index(sa, key);
	if ( index == SIZE_MAX )
		return NULL;
	return sa->strings[index];
}

const char* dictionary_get(string_array_t* sa, const char* key,
                           const char* def = NULL)
{
	size_t keylen = strlen(key);
	const char* entry = dictionary_get_entry(sa, key);
	return entry ? entry + keylen + 1 : def;
}

void dictionary_normalize_entry(char* entry)
{
	bool key = true;
	size_t input_off, output_off;
	for ( input_off = output_off = 0; entry[input_off]; input_off++ )
	{
		if ( key && isspace((unsigned char) entry[input_off]) )
			continue;
		if ( key && (entry[input_off] == '=' || entry[input_off] == '#') )
			key = false;
		entry[output_off++] = entry[input_off];
	}
	entry[output_off] = '\0';
}

void dictionary_append_file(string_array_t* sa, FILE* fp)
{
	char* entry = NULL;
	size_t entry_size = 0;
	ssize_t entry_length;
	while ( 0 < (entry_length = getline(&entry, &entry_size, fp)) )
	{
		if ( entry_length && entry[entry_length-1] == '\n' )
			entry[entry_length-1] = '\0';
		dictionary_normalize_entry(entry);
		if ( entry[0] == '#' )
			continue;
		string_array_append(sa, entry);
	}
	free(entry);
}

bool dictionary_append_file_path(string_array_t* sa, const char* path)
{
	FILE* fp = fopen(path, "r");
	if ( !fp )
		return false;
	dictionary_append_file(sa, fp);
	fclose(fp);
	return true;
}

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

char* read_single_line(FILE* fp)
{
	char* ret = NULL;
	size_t ret_size = 0;
	ssize_t ret_len = getline(&ret, &ret_size, fp);
	if ( ret_len < 0 )
	{
		free(ret);
		return NULL;
	}
	if ( ret_len && ret[ret_len-1] == '\n' )
		ret[ret_len-1] = '\0';
	return ret;
}

pid_t fork_or_death()
{
	pid_t child_pid = fork();
	if ( child_pid < 0 )
		error(1, errno, "fork");
	return child_pid;
}

void waitpid_or_death(pid_t child_pid, bool die_on_error = true)
{
	int status;
	waitpid(child_pid, &status, 0);
	if ( die_on_error )
	{
		if ( WIFEXITED(status) && WEXITSTATUS(status) != 0 )
			exit(WEXITSTATUS(status));
		if ( WIFSIGNALED(status) )
			error(128 + WTERMSIG(status), 0, "child with pid %ju was killed by "
			      "signal %i (%s).", (uintmax_t) child_pid, WTERMSIG(status),
		          strsignal(WTERMSIG(status)));
	}
}

bool fork_and_wait_or_death(bool die_on_error = true)
{
	pid_t child_pid = fork_or_death();
	if ( !child_pid )
		return true;
	waitpid_or_death(child_pid, die_on_error);
	return false;
}

const char* getenv_def(const char* var, const char* def)
{
	const char* ret = getenv(var);
	return ret ? ret : def;
}

int mkdir_p(const char* path, mode_t mode)
{
	int saved_errno = errno;
	if ( mkdir(path, mode) != 0 && errno != EEXIST )
		return -1;
	errno = saved_errno;
	return 0;
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

char* GetBuildTriplet()
{
#if defined(__sortix__) && defined(__i386__)
	return strdup("i486-sortix");
#elif defined(__sortix__) && defined(__x86_64__)
	return strdup("x86_64-sortix");
#elif defined(__sortix__)
	#warning "Add your build triplet here"
#endif
	FILE* fp = popen("cc -dumpmachine", "r");
	if ( !fp )
		return NULL;
	char* ret = read_single_line(fp);
	pclose(fp);
	return ret;
}

bool get_option_variable(const char* option, char** varptr,
                         const char* arg, int argc, char** argv, int* ip,
                         const char* argv0)
{
	size_t option_len = strlen(option);
	if ( strncmp(option, arg, option_len) != 0 )
		return false;
	if ( arg[option_len] == '=' )
	{
		*varptr = strdup(arg + option_len + 1);
		return true;
	}
	if ( arg[option_len] != '\0' )
		return false;
	if ( *ip + 1 == argc )
	{
		fprintf(stderr, "%s: expected operand after `%s'\n", argv0, option);
		exit(1);
	}
	*varptr = strdup(argv[++*ip]), argv[*ip] = NULL;
	return true;
}

char* join_paths(const char* a, const char* b)
{
	size_t a_len = strlen(a);
	bool has_slash = (a_len && a[a_len-1] == '/') || b[0] == '/';
	return has_slash ? print_string("%s%s", a, b) : print_string("%s/%s", a, b);
}

bool IsFile(const char* path)
{
	struct stat st;
	return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

bool IsDirectory(const char* path)
{
	struct stat st;
	return stat(path, &st) == 0 &&
	       (S_ISDIR(st.st_mode) || (errno = ENOTDIR, false));
}

size_t count_tar_components(const char* path)
{
	if ( !*path )
		return 0;
	size_t slashes = 1;
	for ( size_t i = 0; path[i]; i++ )
		if ( path[i] == '/' )
			slashes++;
	return slashes;
}

#define GET_OPTION_VARIABLE(str, varptr) \
        get_option_variable(str, varptr, arg, argc, argv, &i, argv0)

// TODO: This is a bit inefficient but doing it otherwise would involve looking
//       through the error stream for messages such as "file not found", which
//       can be hard to distinguish from the common "oh no, an error occured"
//       case in which we need to abort as well.
bool TarContainsFile(const char* archive, const char* file)
{
	int pipes[2];
	if ( pipe(pipes) )
		error(1, errno, "pipe");
	pid_t tar_pid = fork_or_death();
	if ( !tar_pid )
	{
		dup2(pipes[1], 1);
		close(pipes[1]);
		close(pipes[0]);
		const char* cmd_argv[] =
		{
			"tar",
			"--list",
			"--file", archive,
			NULL
		};
		execvp(cmd_argv[0], (char* const*) cmd_argv);
		error(127, errno, "%s", cmd_argv[0]);
	}
	close(pipes[1]);
	FILE* fp = fdopen(pipes[0], "r");

	char* line = NULL;
	size_t line_size = 0;
	ssize_t line_len;
	bool ret = false;
	while ( 0 < (line_len = getline(&line, &line_size, fp)) )
	{
		if ( line_len && line[line_len-1] == '\n' )
			line[--line_len] = '\0';
		if ( strcmp(line, file) == 0 )
		{
			ret = true;
		#if !defined(__sortix__)
			kill(tar_pid, SIGPIPE);
			break;
		#endif
		}
	}
	free(line);

	fclose(fp);
	int tar_exit_status;
	waitpid(tar_pid, &tar_exit_status, 0);
	bool sigpiped = WIFSIGNALED(tar_exit_status) &&
	                WTERMSIG(tar_exit_status) == SIGPIPE;
	bool errored = !WIFEXITED(tar_exit_status) ||
	               WEXITSTATUS(tar_exit_status) != 0;
	if ( errored && !sigpiped )
	{
		error(1, 0, "Unable to list contents of `%s'.", archive);
		exit(WEXITSTATUS(tar_exit_status));
	}
	return ret;
}

void TarExtractFileToFD(const char* archive, const char* file, int fd)
{
	pid_t tar_pid = fork_or_death();
	if ( !tar_pid )
	{
		if ( dup2(fd, 1) < 0 )
		{
			error(0, errno, "dup2");
			_exit(127);
		}
		close(fd);
		const char* cmd_argv[] =
		{
			"tar",
			"--to-stdout",
			"--extract",
			"--file", archive,
			"--", file,
			NULL
		};
		execvp(cmd_argv[0], (char* const*) cmd_argv);
		error(0, errno, "%s", cmd_argv[0]);
		_exit(127);
	}
	int tar_exit_status;
	waitpid(tar_pid, &tar_exit_status, 0);
	if ( !WIFEXITED(tar_exit_status) || WEXITSTATUS(tar_exit_status) != 0 )
	{
		error(1, 0, "Unable to extract `%s/%s'", archive, file);
		exit(WEXITSTATUS(tar_exit_status));
	}
}

FILE* TarOpenFile(const char* archive, const char* file)
{
	FILE* fp = tmpfile();
	if ( !fp )
		error(1, errno, "tmpfile");
	TarExtractFileToFD(archive, file, fileno(fp));
	if ( fseeko(fp, 0, SEEK_SET) < 0 )
		error(1, errno, "fseeko(tmpfile(), 0, SEEK_SET)");
	return fp;
}

void TarIndexToFD(const char* archive, int fd)
{
	pid_t tar_pid = fork_or_death();
	if ( !tar_pid )
	{
		if ( dup2(fd, 1) < 0 )
		{
			error(0, errno, "dup2");
			_exit(127);
		}
		close(fd);
		const char* cmd_argv[] =
		{
			"tar",
			"--list",
			"--file", archive,
			NULL
		};
		execvp(cmd_argv[0], (char* const*) cmd_argv);
		error(0, errno, "%s", cmd_argv[0]);
		_exit(127);
	}
	int tar_exit_status;
	waitpid(tar_pid, &tar_exit_status, 0);
	if ( !WIFEXITED(tar_exit_status) || WEXITSTATUS(tar_exit_status) != 0 )
	{
		error(1, 0, "Unable to list contents of `%s'", archive);
		exit(WEXITSTATUS(tar_exit_status));
	}
}

FILE* TarOpenIndex(const char* archive)
{
	FILE* fp = tmpfile();
	if ( !fp )
		error(1, errno, "tmpfile");
	TarIndexToFD(archive, fileno(fp));
	if ( fseeko(fp, 0, SEEK_SET) < 0 )
		error(1, errno, "fseeko(tmpfile(), 0, SEEK_SET)");
	return fp;
}

const char* VerifyInfoVariable(string_array_t* info, const char* var,
                               const char* path)
{
	const char* ret = dictionary_get(info, var);
	if ( !ret )
		error(1, 0, "error: `%s': no `%s' variable declared", path, var);
	return ret;
}

void VerifyTixInformation(string_array_t* tixinfo, const char* tix_path)
{
	const char* tix_version = dictionary_get(tixinfo, "tix.version");
	if ( !tix_version )
		error(1, 0, "error: `%s': no `tix.version' variable declared",
		            tix_path);
	if ( atoi(tix_version) != 1 )
		error(1, 0, "error: `%s': tix version `%s' not supported", tix_path,
		            tix_version);
	const char* tix_class = dictionary_get(tixinfo, "tix.class");
	if ( !tix_class )
		error(1, 0, "error: `%s': no `tix.class' variable declared", tix_path);
	if ( !strcmp(tix_class, "srctix") )
		error(1, 0, "error: `%s': this object is a source tix and needs to be "
		            "compiled into a binary tix prior to installation.",
		            tix_path);
	if ( strcmp(tix_class, "tix") )
		error(1, 0, "error: `%s': tix class `%s' is not `tix': this object is "
		            "not suitable for installation.", tix_path, tix_class);
	if ( !(dictionary_get(tixinfo, "tix.platform")) )
		error(1, 0, "error: `%s': no `tix.platform' variable declared", tix_path);
	if ( !(dictionary_get(tixinfo, "pkg.name")) )
		error(1, 0, "error: `%s': no `pkg.name' variable declared", tix_path);
}

bool IsCollectionPrefixRatherThanCommand(const char* arg)
{
	return strchr(arg, '/') || !strcmp(arg, ".") || !strcmp(arg, "..");
}

void ParseOptionalCommandLineCollectionPrefix(char** collection, int* argcp,
                                              char*** argvp)
{
	if ( 2 <= *argcp && IsCollectionPrefixRatherThanCommand((*argvp)[1]) )
	{
		if ( !*collection )
		{
			free(*collection);
			*collection = strdup((*argvp)[1]);
		}
		(*argvp)[1] = NULL;
		compact_arguments(argcp, argvp);
	}
	else if ( !*collection )
	{
		*collection = strdup("/");
	}
}

void VerifyCommandLineCollection(char** collection)
{
	if ( !*collection )
		error(1, 0, "error: you need to specify which tix collection to "
		            "administer using --collection or giving the prefix as the "
		            "first argument.");

	if ( !**collection )
	{
		free(*collection);
		*collection = strdup("/");
	}

	char* collection_rel = *collection;
	if ( !(*collection = canonicalize_file_name(collection_rel)) )
		error(1, errno, "canonicalize_file_name(`%s')", collection_rel);
	free(collection_rel);
}

void VerifyTixCollectionConfiguration(string_array_t* info, const char* path)
{
	const char* tix_version = dictionary_get(info, "tix.version");
	if ( !tix_version )
		error(1, 0, "error: `%s': no `tix.version' variable declared", path);
	if ( atoi(tix_version) != 1 )
		error(1, 0, "error: `%s': tix version `%s' not supported", path,
		            tix_version);
	const char* tix_class = dictionary_get(info, "tix.class");
	if ( !tix_class )
		error(1, 0, "error: `%s': no `tix.class' variable declared", path);
	if ( strcmp(tix_class, "collection") != 0 )
		error(1, 0, "error: `%s': error: unexpected tix class `%s'.", path,
		            tix_class);
	if ( !(dictionary_get(info, "collection.prefix")) )
		error(1, 0, "error: `%s': no `collection.prefix' variable declared",
		            path);
	if ( !(dictionary_get(info, "collection.platform")) )
		error(1, 0, "error: `%s': no `collection.platform' variable declared",
		            path);
}

static pid_t original_pid;

__attribute__((constructor))
static void initialize_original_pid()
{
	original_pid = getpid();
}

void cleanup_file_or_directory(int, void* path_ptr)
{
	if ( original_pid != getpid() )
		return;
	pid_t pid = fork();
	if ( pid < 0 )
	{
		error(0, errno, "fork");
		return;
	}
	if ( pid == 0 )
	{
		const char* cmd_argv[] =
		{
			"rm",
			"-rf",
			"--",
			(const char*) path_ptr,
			NULL,
		};
		execvp(cmd_argv[0], (char* const*) cmd_argv);
		error(0, errno, "%s", cmd_argv[0]);
		_exit(127);
	}
	int code;
	waitpid(pid, &code, 0);
}

mode_t get_umask_value()
{
	mode_t result = umask(0);
	umask(result);
	return result;
}

int fchmod_plus_x(int fd)
{
	struct stat st;
	if ( fstat(fd, &st) != 0 )
		return -1;
	mode_t new_mode = st.st_mode | (0111 & ~get_umask_value());
	if ( fchmod(fd, new_mode) != 0 )
		return -1;
	return 0;
}

void fprint_shell_variable_assignment(FILE* fp, const char* variable, const char* value)
{
	if ( value )
	{
		fprintf(fp, "export %s='", variable);
		for ( size_t i = 0; value[i]; i++ )
			if ( value[i] == '\'' )
				fprintf(fp, "'\\''");
			else
				fputc(value[i], fp);
		fprintf(fp, "'\n");
	}
	else
	{
		fprintf(fp, "unset %s\n", variable);
	}
}

bool is_success_exit_status(int status)
{
	return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

__attribute__((noreturn))
bool exit_like_exit_status(int status)
{
	if ( WIFEXITED(status) )
		exit(WEXITSTATUS(status));
	if ( WIFSIGNALED(status) )
		exit(128 + WTERMSIG(status));
	exit(1);
}

enum recovery_state
{
	RECOVERY_STATE_NONE,
	RECOVERY_STATE_PRINT_COMMAND,
	RECOVERY_STATE_RUN_SHELL,
};

enum recovery_state
recovery_configure_state(bool set,
                         enum recovery_state to_what = RECOVERY_STATE_NONE)
{
	static enum recovery_state recovery_state = RECOVERY_STATE_NONE;
	if ( set )
		recovery_state = to_what;
	return recovery_state;
}

bool recovery_print_attempted_execution()
{
	pid_t child_pid = fork();
	if ( child_pid < 0 )
		return false;

	if ( !child_pid )
	{
		// Redirect stdout and stderr to /dev/null to prevent duplicate errors.
		// The recovery_execvp function below will automatically re-open the
		// terminal device when it needs to talk to the terminal.
		int dev_null = open("/dev/null", O_WRONLY);
		if ( 0 <= dev_null )
		{
			dup2(dev_null, 1);
			dup2(dev_null, 2);
			close(dev_null);
		}

		recovery_configure_state(true, RECOVERY_STATE_PRINT_COMMAND);
		return true;
	}

	int status;
	waitpid(child_pid, &status, 0);

	return false;
}

bool recovery_run_shell()
{
	pid_t child_pid = fork();
	if ( child_pid < 0 )
		return false;

	if ( !child_pid )
	{
		recovery_configure_state(true, RECOVERY_STATE_RUN_SHELL);
		return true;
	}

	int status;
	waitpid(child_pid, &status, 0);

	return false;
}

int recovery_execvp(const char* path, char* const* argv)
{
	if ( recovery_configure_state(false) == RECOVERY_STATE_NONE )
		return execvp(path, argv);

	// Make sure that stdout and stderr go to an interactive terminal.
	if ( !isatty(1) )
	{
		int dev_tty = open("/dev/tty", O_WRONLY);
		if ( 0 <= dev_tty )
		{
			dup2(dev_tty, 1);
			dup2(dev_tty, 2);
			close(dev_tty);
		}
	}

	printf("Attempted command was: ");

	for ( int i = 0; argv[i]; i++ )
	{
		if ( i )
			putchar(' ');
		putchar('\'');
		for ( size_t n = 0; argv[i][n]; n++ )
			if ( argv[i][n] == '\'' )
				printf("'\\''");
			else
				putchar(argv[i][n]);
		putchar('\'');
	}

	printf("\n");

	if ( recovery_configure_state(false) == RECOVERY_STATE_PRINT_COMMAND )
		_exit(0);

	const char* cmd_argv[] =
	{
		getenv_def("SHELL", "sh"),
		NULL
	};
	execvp(cmd_argv[0], (char* const*) cmd_argv);
	error(127, errno, "%s", cmd_argv[0]);

	__builtin_unreachable();
}

bool fork_and_wait_or_recovery()
{
	int default_selection = 1;

	while ( true )
	{
		pid_t child_pid = fork_or_death();
		if ( !child_pid )
			return true;

		int status;
		waitpid(child_pid, &status, 0);

		if ( is_success_exit_status(status) )
			return false;

		if ( WIFEXITED(status) )
			error(0, 0, "child with pid %ju exited with status %i.",
			            (uintmax_t) child_pid, WEXITSTATUS(status));
		else if ( WIFSIGNALED(status) )
			error(0, 0, "child with pid %ju was killed by signal %i (%s).",
			            (uintmax_t) child_pid, WTERMSIG(status),
			            strsignal(WTERMSIG(status)));
		else
			error(0, 0, "child with pid %ju exited in an unusual manner (%i).",
			            (uintmax_t) child_pid, status);

		if ( recovery_print_attempted_execution() )
			return true;

		if ( !isatty(0) )
			exit_like_exit_status(status);

		FILE* output = fopen("/dev/tty", "we");
		if ( !output )
			exit_like_exit_status(status);

retry_ask_recovery_method:

		fprintf(output, "\n");
		fprintf(output, "1. Abort\n");
		fprintf(output, "2. Try again\n");
		fprintf(output, "3. Pretend command was successful\n");
		fprintf(output, "4. Run $SHELL -i to investigate\n");
		fprintf(output, "5. Dump environment\n");
		fprintf(output, "\n");
		fprintf(output, "Please choose one: [%i] ", default_selection);
		fflush(output);

		char* input = read_single_line(stdin);
		if ( !input )
		{
			fprintf(output, "\n");
			fclose(output);
			error(0, errno, "can't read line from standard input, aborting.");
			exit_like_exit_status(status);
		}

		int selection = default_selection;
		if ( input[0] )
		{
			char* input_end;
			selection = (int) strtol(input, &input_end, 0);
			if ( *input_end )
			{
				error(0, 0, "error: `%s' is not an allowed choice", input);
				goto retry_ask_recovery_method;
			}

			if ( 5 < selection )
			{
				error(0, 0, "error: `%i' is not an allowed choice", selection);
				goto retry_ask_recovery_method;
			}
		}

		if ( selection == 1 )
			exit_like_exit_status(status);

		if ( selection == 2 )
		{
			fprintf(output, "\nTrying to execute command again.\n\n");
			fclose(output);
			continue;
		}

		if ( selection == 3 )
		{
			fprintf(output, "\nPretending the command executed successfully.\n\n");
			fclose(output);
			return false;
		}

		if ( selection == 4 )
		{
			fprintf(output, "\nDropping you to a recovery shell, type `exit' "
			                "when you are done.\n\n");
			if ( recovery_run_shell() )
				return true;
		}

		if ( selection == 5 )
		{
			for ( size_t i = 0; environ[i]; i++ )
				fprintf(output, "%s\n", environ[i]);
			goto retry_ask_recovery_method;
		}

		default_selection = 2;

		goto retry_ask_recovery_method;
	}
}

#endif
