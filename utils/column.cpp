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

    column.cpp
    Columnate lists.

*******************************************************************************/

#include <sys/ioctl.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <error.h>
#include <string.h>
#include <termios.h>

#if !defined(VERSIONSTR)
#define VERSIONSTR "unknown version"
#endif

int termwidth = 80;

size_t linesused = 0;
size_t lineslength = 0;
char** lines = 0;
size_t longestline = 0;

size_t measurelength(const char* line)
{
	size_t len = 0;
	bool escaped = false;
	while ( char c = *line++ )
	{
		if ( escaped )
		{
			if ( isalpha(c) )
				escaped = false;
			continue;
		}
		if ( c == '\e' )
		{
			escaped = true;
			continue;
		}
		len++;
	}
	return len;
}

bool processline(char* line)
{
	if ( linesused == lineslength )
	{
		size_t newlineslength = lineslength ? 2 * lineslength : 32;
		char** newlines = new char*[newlineslength];
		memcpy(newlines, lines, sizeof(char*) * linesused);
		delete[] lines;
		lines = newlines;
		lineslength = newlineslength;
	}

	lines[linesused++] = line;
	size_t linelen = measurelength(line);
	if ( longestline < linelen ) { longestline = linelen; }
	return true;
}

bool processfp(const char* inputname, FILE* fp)
{
	if ( strcmp(inputname, "-") == 0 ) { inputname = "<stdin>"; }
	while ( true )
	{
		size_t linesize;
		char* line = NULL;
		ssize_t linelen = getline(&line, &linesize, fp);
		if ( linelen < 0 )
		{
			if ( feof(fp) ) { return true; }
			error(0, errno, "error reading: %s (%i)", inputname, fileno(fp));
			return false;
		}
		line[--linelen] = 0;
		if ( !processline(line) ) { exit(1); }
	}
}

bool doformat()
{
	size_t width = termwidth;
	size_t columnwidth = longestline + 2;
	size_t columns = width / columnwidth;
	if ( !columns ) { columns = 1; }

	bool newline = false;
	for ( size_t i = 0; i < linesused; i++ )
	{
		newline = false;
		size_t column = i % columns;
		size_t offset = column * columnwidth;
		printf("\e[%zuG%s", offset+1, lines[i]);
		if ( columns <= column + 1 ) { printf("\n"); newline = true; }
	}
	if ( linesused && !newline ) { printf("\n"); }

	return true;
}

void help(const char* argv0)
{
	printf("usage: %s [--help | --version] [-c width]  [<FILE> ...]\n", argv0);
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
#if defined(__sortix__)
	struct winsize ws;
	if ( tcgetwinsize(1, &ws) == 0 )
		termwidth = ws.ws_col;
#elif defined(TIOCGWINSZ)
	struct winsize ws;
	if ( ioctl(1, TIOCGWINSZ, &ws) == 0 )
		termwidth = ws.ws_col;
#else
	if ( getenv("COLUMNS") )
		termwidth = atoi(getenv("COLUMNS"));
#endif

	const char* argv0 = argv[0];

	for ( int i = 1; i < argc; i++ )
	{
		const char* arg = argv[i];
		if ( arg[0] != '-' ) { continue; }
		if ( strcmp(arg, "-") == 0 ) { continue; }
		argv[i] = NULL;
		if ( strcmp(arg, "--") == 0 ) { break; }
		if ( strcmp(arg, "--help") == 0 ) { help(argv0); exit(0); }
		if ( strcmp(arg, "--version") == 0 ) { version(argv0); exit(0); }
		if ( strcmp(arg, "-c") == 0 )
		{
			if ( i+1 == argc ) { help(argv0); exit(1); }
			termwidth = atoi(argv[++i]); argv[i] = NULL;
			continue;
		}
		error(0, 0, "unknown option %s", arg);
		help(argv0);
		return 1;
	}

	bool hadany = false;
	for ( int i = 1; i < argc; i++ )
	{
		const char* arg = argv[i];
		if ( !arg ) { continue; }
		bool isstdin = strcmp(arg, "-") == 0;
		FILE* fp = isstdin ? stdin : fopen(arg, "r");
		if ( !fp ) { error(1, errno, "fopen failed: %s", arg); }
		hadany = true;
		if ( !processfp(arg, fp) ) { return 1; }
		if ( fp != stdin ) { fclose(fp); }
	}

	if ( !hadany )
	{
		if ( !processfp("-", stdin) ) { return 1; }
	}

	if ( !doformat() ) { exit(1); }
	return 0;
}
