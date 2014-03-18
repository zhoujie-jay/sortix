/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012.

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

    tail.cpp
    Output the last part of files.

*******************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <error.h>
#include <ctype.h>

#if !defined(VERSIONSTR)
#define VERSIONSTR "unknown version"
#endif

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
	const bool tail = TAIL;
	const bool head = !TAIL;
	if ( !numlines ) { return true; }
	bool specialleading = (head && 0 < numlines) || (tail && numlines < 0);
	bool specialtrailing = !specialleading;
	long abslines = (numlines < 0) ? -numlines : numlines;

	char** buffer = NULL;
	if ( specialtrailing )
	{
		buffer = new char*[abslines];
		memset(buffer, 0, sizeof(char*) * abslines);
	}

	long linenum;
	for ( linenum = 0; true; linenum++ )
	{
		char* line = NULL;
		size_t linesize;
		ssize_t linelen = getline(&line, &linesize, fp);
		if ( linelen < 0 )
		{
			if ( feof(fp) ) { break; }
			error(1, errno, "error reading line: %s", inputname);
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

	delete[] buffer;

	return true;
}

void help(const char* argv0)
{
	printf("usage: %s [-n <numlines>] [-q | -v] [<FILE> ...]\n", argv0);
}

void errusage(const char* argv0)
{
	help(argv0);
	exit(1);
}

void version(const char* argv0)
{
	FILE* fp = stdout;
	fprintf(fp, "%s (Sortix) %s\n", argv0, VERSIONSTR);
	fprintf(fp, "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n");
	fprintf(fp, "This is free software: you are free to change and redistribute it.\n");
	fprintf(fp, "There is NO WARRANTY, to the extent permitted by law.\n");
}

int main(int argc, char* argv[])
{
	const char* argv0 = argv[0];

	for ( int i = 1; i < argc; i++ )
	{
		const char* arg = argv[i];
		if ( arg[0] != '-' ) { continue; }
		argv[i] = NULL;
		if ( strcmp(arg, "--") == 0 ) { break; }
		const char* nlinesstr = NULL;
		if ( strcmp(arg, "--help") == 0 ) { help(argv0); return 0; }
		if ( strcmp(arg, "--version") == 0 ) { version(argv0); return 0; }
		if ( strcmp(arg, "-q") == 0 ||
		     strcmp(arg, "--quiet") == 0 ||
		     strcmp(arg, "--silent") == 0 )
		{
			quiet = true;
			verbose = false;
			continue;
		}
		if ( strcmp(arg, "-v") == 0 ||
		     strcmp(arg, "--verbose") == 0 )
		{
			quiet = false;
			verbose = true;
			continue;
		}
		if ( strcmp(arg, "-n") == 0 ) { nlinesstr = argv[++i]; argv[i] = NULL; }
		if ( isdigit(arg[1]) ) { nlinesstr = arg+1; }
		if ( nlinesstr )
		{
			char* nlinesstrend;
			long nlines = strtol(nlinesstr, &nlinesstrend, 10);
			if ( *nlinesstrend )
			{
				fprintf(stderr, "Bad number of lines: %s\n", nlinesstr);
				errusage(argv0);
			}
			numlines = nlines;
			continue;
		}
		fprintf(stderr, "%s: unrecognized option '%s'\n", argv0, arg);
		errusage(argv0);
	}

	size_t numfiles = 0;
	for ( int i = 1; i < argc; i++ ) { if ( argv[i] ) { numfiles++; } }

	if ( !numfiles )
	{
		bool header = verbose;
		if ( header ) { printf("==> %s <==\n", STDINNAME); }
		if ( !processfp(STDINNAME, stdin) ) { return 1; }
		return 0;
	}

	const char* prefix = "";
	for ( int i = 1; i < argc; i++ )
	{
		if ( !argv[i] ) { continue; }
		bool isstdin = strcmp(argv[i], "-") == 0;
		FILE* fp = isstdin ? stdin : fopen(argv[i], "r");
		if ( !fp ) { error(1, errno, "error opening: %s", argv[i]); }
		bool header = !quiet && (verbose || 1 < numfiles);
		if ( header ) { printf("%s==> %s <==\n", prefix, argv[i]); }
		prefix = "\n";
		if ( !processfp(isstdin ? STDINNAME : argv[i], fp) ) { return 1; }
		if ( !isstdin ) { fclose(fp); }
	}

	return 0;
}
