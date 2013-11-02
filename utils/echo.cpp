/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2013.

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

    echo.cpp
    Write arguments to standard output.

*******************************************************************************/

#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if !defined(VERSIONSTR)
#define VERSIONSTR "unknown version"
#endif

static void help(FILE* fp, const char* argv0)
{
	fprintf(fp, "Usage: %s [SHORT-OPTION]... [STRING]...\n", argv0);
	fprintf(fp, "  or:  %s [LONG-OPTION]\n", argv0);
	fprintf(fp, "Echo the STRING(s) to standard output.\n");
	fprintf(fp, "\n");
	fprintf(fp, "  -n             do not output the trailing newline\n");
	fprintf(fp, "  -e             enable interpretation of backslash escapes\n");
	fprintf(fp, "  -E             disable interpretation of backslash escapes (default)\n");
	fprintf(fp, "      --help     display this help and exit\n");
	fprintf(fp, "      --version  output version information and exit\n");
	fprintf(fp, "\n");
	fprintf(fp, "If -e is in effect, the following sequences are recognized:\n");
	fprintf(fp, "\n");
	fprintf(fp, "  \\\\      backslash\n");
	fprintf(fp, "  \\a      alert (BEL)\n");
	fprintf(fp, "  \\b      backspace\n");
	fprintf(fp, "  \\c      produce no further output\n");
	fprintf(fp, "  \\e      escape\n");
	fprintf(fp, "  \\f      form feed\n");
	fprintf(fp, "  \\n      new line\n");
	fprintf(fp, "  \\r      carriage return\n");
	fprintf(fp, "  \\t      horizontal tab\n");
	fprintf(fp, "  \\v      vertical tab\n");
	fprintf(fp, "  \\0NNN   byte with octal value NNN (1 to 3 digits)\n");
	fprintf(fp, "  \\xHH    byte with hexadecimal value HH (1 to 2 digits)\n");
	fprintf(fp, "\n");
}

static void version(FILE* fp, const char* argv0)
{
	fprintf(fp, "%s (Sortix) %s\n", argv0, VERSIONSTR);
	fprintf(fp, "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n");
	fprintf(fp, "This is free software: you are free to change and redistribute it.\n");
	fprintf(fp, "There is NO WARRANTY, to the extent permitted by law.\n");
}

static bool is_allowed_short_argument(const char* argument)
{
	if ( argument[0] != '-' )
		return false;
	if ( !argument[1] )
		return false;
	for ( size_t i = 1; argument[i]; i++ )
	{
		switch ( argument[i] )
		{
		case 'e': break;
		case 'E': break;
		case 'n': break;
		default: return false;
		}
	}
	return true;
}

int main(int argc, char* argv[])
{
	bool newline = true;
	bool escape_sequences = false;

	if ( 2 == argc && !strcmp(argv[1], "--help") )
		help(stdout, argv[0]), exit(0);
	if ( 2 == argc && !strcmp(argv[1], "--version") )
		version(stdout, argv[0]), exit(0);

	for ( int i = 1; i < argc; i++ )
	{
		const char* arg = argv[i];
		if ( !is_allowed_short_argument(arg) )
			break;
		argv[i] = NULL;
		for ( size_t n = 1; arg[n]; n++ )
		{
			switch ( arg[n] )
			{
			case 'e': escape_sequences = true; break;
			case 'E': escape_sequences = false; break;
			case 'n': newline = false; break;
			default: break;
			}
		}
	}

	const char* prefix = "";
	for ( int i = 1; i < argc; i++ )
	{
		const char* arg = argv[i];
		if ( !arg )
			continue;
		if ( !escape_sequences )
		{
			if ( printf("%s%s", prefix, arg) < 0 )
				error(1, errno, "<stdout>");
		}
		else
		{
			if ( printf("%s", prefix) < 0 )
				error(1, errno, "<stdout>");
			bool escaped = false;
			for ( size_t i = 0; arg[i]; i++ )
			{
				char c = arg[i];
				if ( escaped && (escaped = false, true) )
				{
					if ( c == '0' && (i++, true) )
					{
						int value = 0;
						if ( '0' <= arg[i] && arg[i] <= '7' )
							value = value * 8 + arg[i] - '0', i++;
						if ( '0' <= arg[i] && arg[i] <= '7' )
							value = value * 8 + arg[i] - '0', i++;
						if ( '0' <= arg[i] && arg[i] <= '7' )
							value = value * 8 + arg[i] - '0', i++;
						if ( 255 < value )
							value &= 255;
						if ( putchar(value) == EOF )
							error(1, errno, "<stdout>");
						i--;
						continue;
					}
					if ( c == 'x' && (i++, true) )
					{
						int value = 0;
						if ( '0' <= arg[i] && arg[i] <= '7' )
							value = value * 16 + arg[i] - '0', i++;
						else if ( 'a' <= arg[i] && arg[i] <= 'f' )
							value = value * 16 + arg[i] - 'a' + 10, i++;
						else if ( 'A' <= arg[i] && arg[i] <= 'F' )
							value = value * 16 + arg[i] - 'A' + 10, i++;
						if ( '0' <= arg[i] && arg[i] <= '7' )
							value = value * 16 + arg[i] - '0', i++;
						else if ( 'a' <= arg[i] && arg[i] <= 'f' )
							value = value * 16 + arg[i] - 'a' + 10, i++;
						else if ( 'A' <= arg[i] && arg[i] <= 'F' )
							value = value * 16 + arg[i] - 'A' + 10, i++;
						if ( putchar(value) == EOF )
							error(1, errno, "<stdout>");
						i--;
						continue;
					}
					switch ( c )
					{
					case '\\': c = '\\'; break;
					case 'a': c = '\a'; break;
					case 'b': c = '\b'; break;
					case 'c': goto no_futher_output;
					case 'e': c = '\e'; break;
					case 'f': c = '\f'; break;
					case 'n': c = '\n'; break;
					case 'r': c = '\r'; break;
					case 't': c = '\t'; break;
					case 'v': c = '\v'; break;
					default: break;
					}
				}
				else if ( c == '\\' )
				{
					escaped = true;
					continue;
				}
				if ( putchar((unsigned char) c) == EOF )
					error(1, errno, "<stdout>");
			}
			if ( escaped && putchar('\\') == EOF )
				error(1, errno, "<stdout>");
		}
		prefix = " ";
	}

	if ( newline && printf("\n") < 0 )
		error(1, errno, "<stdout>");

no_futher_output:
	if ( fflush(stdout) != 0 || fsync(1) != 0 )
		error(1, errno, "<stdout>");

	return 0;
}
