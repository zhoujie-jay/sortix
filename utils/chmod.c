/*
 * Copyright (c) 2014 Jonas 'Sortie' Termansen.
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
 * chmod.c
 * Change file mode bits.
 */

#include <sys/types.h>
#include <sys/stat.h>

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const int FLAG_CHANGES = 1 << 0;
static const int FLAG_VERBOSE = 1 << 1;
static const int FLAG_RECURSIVE = 1 << 2;

enum symderef
{
	SYMDEREF_NONE,
	SYMDEREF_ARGUMENTS,
	SYMDEREF_ALWAYS,
	SYMDEREF_DEFAULT,
};

static bool is_octal_string(const char* str)
{
	if ( !str[0] )
		return false;
	for ( size_t i = 0; str[i]; i++ )
		if ( !('0' <= str[i] && str[i] <= '7') )
			return false;
	return true;
}

static mode_t execute_modespec(const char* str,
                               mode_t mode,
                               mode_t type,
                               mode_t umask)
{
	if ( is_octal_string(str) )
	{
		errno = 0;
		uintmax_t input = strtoumax((char*) str, NULL, 8);
		if ( errno == ERANGE )
			return (mode_t) -1;
		if ( input & ~((uintmax_t) 07777) )
			return (mode_t) -1;
		return (mode_t) input;
	}

	size_t index = 0;
	do
	{
		mode_t who_mask = 01000;
		while ( true )
		{
			if ( str[index] == 'u' && (index++, true) )
				who_mask |= 04700;
			else if ( str[index] == 'g' && (index++, true) )
				who_mask |= 02070;
			else if ( str[index] == 'o' && (index++, true) )
				who_mask |= 00007;
			else if ( str[index] == 'a' && (index++, true) )
				who_mask |= 06777;
			else
				break;
		}
		if ( !(who_mask & 0777) )
			who_mask |= 06777 & ~umask;
		do
		{
			char op;
			switch ( (op = str[index++]) )
			{
			case '+': break;
			case '-': break;
			case '=': break;
			default: return (mode_t) -1;
			};
			mode_t operand = 0;
			if ( str[index] == 'u' || str[index] == 'g' || str[index] == 'o' )
			{
				char permcopy = str[index++];
				switch ( permcopy )
				{
				case 'u': operand = mode >> 6 & 07; break;
				case 'g': operand = mode >> 3 & 07; break;
				case 'o': operand = mode >> 0 & 07; break;
				default: __builtin_unreachable();
				};
				operand = operand << 0 | operand << 3 | operand << 6;
				switch ( permcopy )
				{
				case 'u': if ( mode & 04000) operand |= 06000; break;
				case 'g': if ( mode & 02000) operand |= 06000; break;
				};
				who_mask &= ~((mode_t) 01000);
			}
			else
			{
				bool unknown = false;
				do
				{
					switch ( str[index] )
					{
					case 'r': operand |= 00444; break;
					case 'w': operand |= 00222; break;
					case 'x': operand |= 00111; break;
					case 'X':
						if ( S_ISDIR(type) || (mode & 0111) )
							operand |= 00111;
						break;
					case 's': operand |= 06000; break;
					case 't': operand |= 00000; break;
					default: unknown = true; break;
					}
				} while ( !unknown && (index++, true) );
			}
			switch ( op )
			{
			case '+': mode |= (operand & who_mask); break;
			case '-': mode &= ~(operand & who_mask); break;
			case '=': mode = (mode & ~who_mask) | (operand & who_mask); break;
			default: __builtin_unreachable();
			}
		} while ( str[index] == '+' ||
		          str[index] == '-' ||
		          str[index] == '=' );
	} while ( str[index] == ',' && (index++, true) );
	if ( str[index] )
		return (mode_t) -1;
	return mode;
}

static bool is_valid_modespec(const char* str)
{
	return execute_modespec(str, 0, 0, 0) != (mode_t) -1;
}

static bool do_chmod_directory(int fd,
                               const char* path,
                               const char* modespec,
                               int flags,
                               enum symderef symderef);

static bool do_chmod(int dirfd,
                     const char* relpath,
                     const char* path,
                     const char* modespec,
                     int flags,
                     enum symderef symderef)
{
	bool success = true;

	int fd_open_flags = O_RDONLY;
	if ( symderef == SYMDEREF_NONE )
		fd_open_flags |= O_SYMLINK_NOFOLLOW;
	int fd = openat(dirfd, relpath, fd_open_flags);
	if ( fd < 0 )
	{
		error(0, errno, "`%s'", path);
		return false;
	}

	struct stat st;
	if ( fstat(fd, &st) < 0 )
	{
		error(0, errno, "stat: `%s'", path);
		close(fd);
		return false;
	}

	if ( S_ISLNK(st.st_mode) )
	{
		close(fd);
		return true;
	}

	mode_t old_mode = st.st_mode & 07777;
	mode_t new_mode = execute_modespec(modespec, old_mode, st.st_mode, getumask());
	assert(new_mode != (mode_t) -1);

	if ( fchmod(fd, new_mode) < 0 )
	{
		error(0, errno, "changing permissions: `%s'", path);
		success = false;
	}
	else
	{
		mode_t naive_mode = execute_modespec(modespec, old_mode, st.st_mode, 0);
		assert(naive_mode != (mode_t) -1);

		if ( new_mode & ~naive_mode )
			error(0, 0, "%s: new permissions are %jo, not %jo",
			            path, (uintmax_t) new_mode, (uintmax_t) naive_mode);

		if ( (flags & FLAG_VERBOSE) ||
			 (old_mode != new_mode && (flags & FLAG_CHANGES)) )
		{
			if ( old_mode == new_mode )
				printf("mode of `%s' retained as %jo\n",
					   path, (uintmax_t) new_mode);
			else
				printf("mode of `%s' changed from %jo to %jo\n",
					   path, (uintmax_t) old_mode, (uintmax_t) new_mode);
		}
	}

	if ( S_ISDIR(st.st_mode) && (flags & FLAG_RECURSIVE) )
	{
		if ( !do_chmod_directory(fd, path, modespec, flags, symderef) )
			success = false;
	}

	close(fd);
	return success;
}

static bool do_chmod_directory(int fd,
                               const char* path,
                               const char* modespec,
                               int flags,
                               enum symderef symderef)
{
	if ( symderef == SYMDEREF_ARGUMENTS )
		symderef = SYMDEREF_NONE;

	int fd_copy = dup(fd);
	if ( fd_copy < 0 )
	{
		error(0, errno, "dup: `%s'", path);
		return false;
	}

	DIR* dir = fdopendir(fd_copy);
	if ( !dir )
	{
		error(0, errno, "fdopendir: `%s'", path);
		close(fd_copy);
		return false;
	}

	const char* joiner = "/";
	size_t path_length = strlen(path);
	if ( path_length && path[path_length - 1] == '/' )
		joiner = "";

	bool success = true;
	struct dirent* entry;
	while ( (errno = 0, entry = readdir(dir)) )
	{
		if ( !strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..") )
			continue;

		char* entry_path;
		if ( asprintf(&entry_path, "%s%s%s", path, joiner, entry->d_name) < 0 )
		{
			error(0, errno, "asprintf: `%s%s%s'", path, joiner, entry->d_name);
			success = false;
			continue;
		}

		if ( !do_chmod(fd, entry->d_name, entry_path, modespec, flags, symderef) )
			success = false;

		free(entry_path);
	}

	if ( errno != 0 )
	{
		error(0, errno, "reading directory: `%s'", path);
		closedir(dir);
		return false;
	}

	closedir(dir);
	return success;
}

static bool is_ambiguous_option(const char* str)
{
	if ( str[0] != '-' )
		return false;
	if ( str[1] == '-' )
		return false;
	return is_valid_modespec(str);
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
	fprintf(fp, "Usage: %s [OPTION]... MODE[,MODE]... FILE\n", argv0);
	fprintf(fp, "  or:  %s [OPTION]... OCTAL-MODE FILE...\n", argv0);
#if 0
	fprintf(fp, "  or:  %s [OPTION]... --reference=RFILE FILE...\n", argv0);
#endif
}

static void version(FILE* fp, const char* argv0)
{
	fprintf(fp, "%s (Sortix) %s\n", argv0, VERSIONSTR);
}

int main(int argc, char* argv[])
{
	int flags = 0;
	enum symderef symderef = SYMDEREF_DEFAULT;

	const char* argv0 = argv[0];
	for ( int i = 1; i < argc; i++ )
	{
		const char* arg = argv[i];
		if ( arg[0] != '-' || !arg[1] )
			continue;
		if ( !strcmp(arg, "--") )
			break;
		if ( is_ambiguous_option(arg) )
			continue;
		argv[i] = NULL;
		if ( arg[1] != '-' )
		{
			char c;
			while ( (c = *++arg) ) switch ( c )
			{
			case 'c': flags |= FLAG_CHANGES; break;
			case 'H': symderef = SYMDEREF_ARGUMENTS; break;
			case 'L': symderef = SYMDEREF_ALWAYS; break;
			case 'P': symderef = SYMDEREF_NONE; break;
			case 'R': flags |= FLAG_RECURSIVE; break;
			case 'v': flags |= FLAG_VERBOSE; break;
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
		else if ( !strcmp(arg, "--changes") )
			flags |= FLAG_CHANGES;
		else if ( !strcmp(arg, "--verbose") )
			flags |= FLAG_VERBOSE;
		else if ( !strcmp(arg, "--recursive") )
			flags |= FLAG_RECURSIVE;
		else
		{
			fprintf(stderr, "%s: unknown option: %s\n", argv0, arg);
			help(stderr, argv0);
			exit(1);
		}
	}

	compact_arguments(&argc, &argv);

	if ( flags & FLAG_RECURSIVE )
	{
		if ( symderef == SYMDEREF_DEFAULT )
			symderef = SYMDEREF_NONE;
	}
	else
	{
		symderef = SYMDEREF_NONE;
	}

	if ( argc == 1 )
		error(1, 0, "missing operand");

	const char* modespec = argv[1];

	if ( !is_valid_modespec(modespec) )
		error(1, 0, "invalid mode: `%s'", modespec);

	if ( argc == 2 )
		error(1, 0, "missing operand after `%s'", modespec);

	bool success = true;
	for ( int i = 2; i < argc; i++ )
	{
		const char* arg = argv[i];
		if ( !do_chmod(AT_FDCWD, arg, arg, modespec, flags, symderef) )
			success = false;
	}

	return success ? 0 : 1;
}
