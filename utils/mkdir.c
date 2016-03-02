/*
 * Copyright (c) 2012, 2015 Jonas 'Sortie' Termansen.
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
 * mkdir.c
 * Create a directory.
 */

#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>
#include <error.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static void make_directory(const char* path,
                           bool parents,
                           bool verbose,
                           const char* modespec,
                           bool is_parent)
{
	mode_t my_umask = getumask();
	mode_t mode = 0777;
	if ( modespec )
	{
		mode = execute_modespec(modespec, 0777, S_IFDIR, my_umask);
		if ( is_parent )
			mode |= 0300;
	}
	while ( true )
	{
		if ( modespec )
			umask(0);
		else if ( is_parent && (my_umask & 0300) )
			umask(my_umask & ~0300);
		int ret = mkdir(path, mode);
		if ( modespec || (is_parent && (my_umask & 0300)) )
			umask(my_umask);
		if ( ret < 0 )
		{
			if ( parents )
			{
				if ( errno == EEXIST )
					return;
				if ( errno == ENOENT )
				{
					char* path_copy = strdup(path);
					if ( !path_copy )
						error(1, errno, "malloc");
					char* last_slash = strrchr(path_copy, '/');
					if ( last_slash )
					{
						*last_slash = 0;
						if ( *path_copy )
						{
							make_directory(path_copy, parents, verbose,
							               modespec, true);
							free(path_copy);
							continue;
						}
					}
					free(path_copy);
				}
			}
			error(1, errno, "cannot create directory `%s'", path);
		}
		break;
	}
	if ( verbose )
		fprintf(stderr, "%s: created directory `%s'\n", program_invocation_name, path);
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
	fprintf(fp, "Usage: %s [OPTION]... DIRECTORY...\n", argv0);
	fprintf(fp, "Create directories if they don't already exist.\n");
}

static void version(FILE* fp, const char* argv0)
{
	fprintf(fp, "%s (Sortix) %s\n", argv0, VERSIONSTR);
}

int main(int argc, char* argv[])
{
	const char* argv0 = argv[0];
	bool parents = false;
	bool verbose = false;
	const char* modespec = NULL;

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
			char c;
			while ( (c = *++arg) ) switch ( c )
			{
			case 'm':
				if ( !*(modespec = arg + 1) )
				{
					if ( i + 1 == argc )
					{
						error(0, 0, "option requires an argument -- 'm'");
						fprintf(stderr, "Try `%s --help' for more information.\n", argv[0]);
						exit(125);
					}
					modespec = argv[i+1];
					argv[++i] = NULL;
				}
				arg = "m";
				break;
			case 'p': parents = true; break;
			case 'v': verbose = true; break;
			default:
				fprintf(stderr, "%s: unknown option -- '%c'\n", argv0, c);
				help(stderr, argv0);
				exit(1);
			}
		}
		else if ( !strncmp(arg, "--modespec=", strlen("--modespec=")) )
			modespec = arg + strlen("--modespec=");
		else if ( !strcmp(arg, "--modespec") )
		{
			if ( i + 1 == argc )
			{
				error(0, 0, "option '--modespec' requires an argument");
				fprintf(stderr, "Try `%s --help' for more information.\n", argv[0]);
				exit(125);
			}
			modespec = argv[i+1];
			argv[++i] = NULL;
		}
		else if ( !strcmp(arg, "--parents") )
			parents = true;
		else if ( !strcmp(arg, "--verbose") )
			verbose = true;
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

	if ( argc < 2 )
		error(1, 0, "missing operand");

	if ( modespec && !is_valid_modespec(modespec) )
		error(1, 0, "invalid mode: `%s'", modespec);

	for ( int i = 1; i < argc; i++ )
	{
		const char* arg = argv[i];
		if ( !arg )
			continue;
		make_directory(arg, parents, verbose, modespec, false);
	}

	return 0;
}
