/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

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

    pwd.cpp
    Prints the current working directory.

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

static bool nul_or_slash(char c)
{
	return !c || c == '/';
}

static bool next_elem_is_dot_or_dot_dot(const char* path)
{
	return (path[0] == '.' && nul_or_slash(path[1])) ||
	       (path[0] == '.' && path[1] == '.' && nul_or_slash(path[2]));
}

static bool is_path_absolute(const char* path)
{
	size_t index = 0;
	if ( path[index++] != '/' )
		return false;
	while ( path[index] )
	{
		if ( next_elem_is_dot_or_dot_dot(path) )
			return false;
		while ( !nul_or_slash(path[index]) )
			index++;
		if ( path[index] == '/' )
			index++;
	}
	return true;
}

static void help(FILE* fp, const char* argv0)
{
	fprintf(fp, "Usage: %s [OPTION]...\n", argv0);
	fprintf(fp, "Print the full filename of the current working directory.\n");
	fprintf(fp, "\n");
	fprintf(fp, "  -L, --logical   use PWD from environment, even if it contains symlinks\n");
	fprintf(fp, "  -P, --physical  avoid all symlinks\n");
	fprintf(fp, "      --help      display this help and exit\n");
	fprintf(fp, "      --version   output version information and exit\n");
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
	const char* argv0 = argv[0];
	bool physical = false;
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
			case 'L': physical = false; break;
			case 'P': physical = true; break;
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
		else if ( !strcmp(arg, "--logical") )
			physical = false;
		else if ( !strcmp(arg, "--physical") )
			physical = true;
		else
		{
			fprintf(stderr, "%s: unknown option: %s\n", argv0, arg);
			help(stderr, argv0);
			exit(1);
		}
	}

	// The get_current_dir_name function will use the PWD variable if it is
	// accurate, so we'll need to unset it if it is inappropriate to use it.
	if ( const char* pwd = getenv("PWD") )
	{
		if ( physical || !is_path_absolute(pwd) )
			unsetenv(pwd);
	}

	char* pwd = get_current_dir_name();
	if ( !pwd )
		error(1, errno, "get_current_dir_name");

	printf("%s\n", pwd);

	return 0;
}
