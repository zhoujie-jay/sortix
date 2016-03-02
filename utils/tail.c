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
 * tail.c
 * Output the last part of files.
 */

#include <sys/stat.h>

#include <ctype.h>
#include <errno.h>
#include <error.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HEAD
#define TAIL false
#else
#define TAIL true
#endif

const char* STDINNAME = "standard input";

long numlines = 10;
bool quiet = false;
bool verbose = false;

bool processfp(const char* inputname, FILE* fp)
{
	struct stat st;
	if ( fstat(fileno(fp), &st) == 0 && S_ISDIR(st.st_mode) )
	{
		error(0, EISDIR, "%s", inputname);
		return false;
	}

	const bool tail = TAIL;
	const bool head = !TAIL;
	if ( !numlines ) { return true; }
	bool specialleading = (head && 0 < numlines) || (tail && numlines < 0);
	bool specialtrailing = !specialleading;
	long abslines = (numlines < 0) ? -numlines : numlines;

	char** buffer = NULL;
	if ( specialtrailing )
		buffer = (char**) calloc(1, sizeof(char*) * abslines);

	long linenum;
	for ( linenum = 0; true; linenum++ )
	{
		char* line = NULL;
		size_t linesize;
		ssize_t linelen = getline(&line, &linesize, fp);
		if ( linelen < 0 )
		{
			free(line);
			if ( feof(fp) ) { break; }
			error(1, errno, "%s", inputname);
		}
		if ( specialleading )
		{
			bool doprint = false;
			if ( head && linenum < abslines ) { doprint = true; }
			else if ( tail && abslines <= linenum ) { doprint = true; }
			bool done = head && abslines <= linenum+1;
			if ( doprint ) { printf("%s", line); }
			free(line);
			if ( done ) { return true; }
		}
		if ( specialtrailing )
		{
			long index = linenum % abslines;
			if ( buffer[index] )
			{
				char* bufline = buffer[index];
				if ( head ) { printf("%s", bufline); }
				free(bufline);
			}
			buffer[index] = line;
		}
	}

	if ( specialtrailing && tail )
	{
		long numtoprint = linenum < abslines ? linenum : abslines;
		for ( long i = 0; i < numtoprint; i++ )
		{
			long index = (linenum - numtoprint + i) % abslines;
			printf("%s", buffer[index]);
			free(buffer[index]);
		}
	}

	free(buffer);

	return true;
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
	fprintf(fp, "Usage: %s [OPTION]... [FILE]...\n", argv0);
}

static void version(FILE* fp, const char* argv0)
{
	fprintf(fp, "%s (Sortix) %s\n", argv0, VERSIONSTR);
}

int main(int argc, char* argv[])
{
	const char* nlinesstr = NULL;

	const char* argv0 = argv[0];
	for ( int i = 1; i < argc; i++ )
	{
		const char* arg = argv[i];
		if ( arg[0] != '-' || !arg[1] )
			continue;
		argv[i] = NULL;
		if ( !strcmp(arg, "--") )
			break;
		if ( isdigit((unsigned char) arg[1]) )
			nlinesstr = arg + 1;
		else if ( arg[1] != '-' )
		{
			char c;
			while ( (c = *++arg) ) switch ( c )
			{
			case 'n':
				if ( !*(nlinesstr = arg + 1) )
				{
					if ( i + 1 == argc )
					{
						error(0, 0, "option requires an argument -- 'n'");
						fprintf(stderr, "Try `%s --help' for more information.\n", argv[0]);
						exit(125);
					}
					nlinesstr = argv[i+1];
					argv[++i] = NULL;
				}
				arg = "n";
				break;
			case 'q': quiet = true; verbose = false; break;
			case 'v': quiet = false; verbose = true; break;
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
		else if ( !strcmp(arg, "--quiet") || !strcmp(arg, "--silent") )
			quiet = true, verbose = false;
		else if ( !strcmp(arg, "--verbose") )
			quiet = false, verbose = true;
		else
		{
			fprintf(stderr, "%s: unknown option: %s\n", argv0, arg);
			help(stderr, argv0);
			exit(1);
		}
	}

	compact_arguments(&argc, &argv);

	if ( nlinesstr )
	{
		char* nlinesstrend;
		long nlines = strtol(nlinesstr, &nlinesstrend, 10);
		if ( *nlinesstrend )
			error(1, 0, "Bad number of lines: %s", nlinesstr);
		numlines = nlines;
	}

	if ( argc < 2 )
	{
		bool header = verbose;
		if ( header )
			printf("==> %s <==\n", STDINNAME);
		if ( !processfp(STDINNAME, stdin) )
			return 1;
		return 0;
	}

	int result = 0;

	const char* prefix = "";
	for ( int i = 1; i < argc; i++ )
	{
		bool isstdin = strcmp(argv[i], "-") == 0;
		FILE* fp = isstdin ? stdin : fopen(argv[i], "r");
		if ( !fp )
		{
			error(0, errno, "%s", argv[i]);
			result = 1;
			continue;
		}
		bool header = !quiet && (verbose || 2 < argc);
		if ( header )
			printf("%s==> %s <==\n", prefix, argv[i]);
		prefix = "\n";
		if ( !processfp(isstdin ? STDINNAME : argv[i], fp) )
			result = 1;
		if ( !isstdin )
			fclose(fp);
	}

	return result;
}
