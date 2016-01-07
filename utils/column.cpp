/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012, 2014.

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

#include <errno.h>
#include <error.h>
#include <locale.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <wchar.h>

struct line
{
	const char* string;
	size_t display_width;
};

size_t measure_line_display_width(const char* string)
{
	mbstate_t ps;
	memset(&ps, 0, sizeof(ps));

	size_t index = 0;
	size_t result = 0;

	bool escaped = false;

	while ( true )
	{
		wchar_t wc;
		size_t consumed = mbrtowc(&wc, string + index, SIZE_MAX, &ps);
		if ( consumed == (size_t) -2 )
			return result + 1;
		if ( consumed == (size_t) -1 )
		{
			index++;
			if ( !escaped )
				result++;
			memset(&ps, 0, sizeof(ps));
			continue;
		}
		if ( consumed == (size_t) 0 )
			return result;
		index += consumed;
		if ( wc == L'\e' )
		{
			escaped = true;
			continue;
		}
		if ( escaped )
		{
			if ( ('a' <= wc && wc <= 'z') || ('A' <= wc && wc <= 'Z') )
				escaped = false;
			continue;
		}
		int wc_width = wcwidth(wc);
		result += 0 <= wc_width ? wc_width : 1;
	}
}

bool append_lines_from_file(FILE* fp,
                            const char* fpname,
                            struct line** lines_ptr,
                            size_t* lines_used_ptr,
                            size_t* lines_length_ptr,
                            bool flag_empty)
{
	while ( true )
	{
		char* string = NULL;
		size_t string_size;
		errno = 0;
		ssize_t string_length = getline(&string, &string_size, fp);
		if ( string_length < 0 )
		{
			free(string);
			if ( feof(stdin) )
				return true;
			if ( errno == 0 )
				return true;
			error(0, errno, "getline: `%s'", fpname);
			return false;
		}
		if ( string_length && string[string_length-1] == '\n' )
			string[--string_length] = '\0';
		size_t display_width = measure_line_display_width(string);
		if ( display_width == 0 && !flag_empty )
		{
			free(string);
			continue;
		}
		if ( *lines_used_ptr == *lines_length_ptr )
		{
			size_t new_length = 2 * *lines_length_ptr;
			if ( !new_length )
				new_length = 64;
			size_t new_size = sizeof(struct line) * new_length;
			struct line* new_lines = (struct line*) realloc(*lines_ptr, new_size);
			if ( !new_lines )
			{
				error(0, errno, "realloc lines array");
				free(string);
				return false;
			}
			*lines_ptr = new_lines;
			*lines_length_ptr = new_length;
		}
		struct line* line = (*lines_ptr) + (*lines_used_ptr)++;
		line->string = string;
		line->display_width = display_width;
	}
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
	fprintf(fp, "Usage: %s [OPTION]... FILE...\n", argv0);
	fprintf(fp, "Columnate lists.\n");
	fprintf(fp, "\n");
	fprintf(fp, "  -c COLUMNS                   output is formatted for a display COLUMNS wide.\n");
	fprintf(fp, "  -e                           do not ignore empty lines..\n");
	fprintf(fp, "      --help                   display this help and exit\n");
	fprintf(fp, "      --version                output version information and exit\n");
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
	setlocale(LC_ALL, "");

	size_t display_columns = 80;
#if defined(TIOCGWINSZ)
	struct winsize ws;
	if ( ioctl(1, TIOCGWINSZ, &ws) == 0 )
		display_columns = ws.ws_col;
#elif defined(__sortix__)
	struct winsize ws;
	if ( tcgetwinsize(1, &ws) == 0 )
		display_columns = ws.ws_col;
#endif

	bool flag_empty = false;

	const char* argv0 = argv[0];
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
			case 'c':
			{
				const char* parameter;
				if ( *(arg + 1) )
					parameter = arg + 1;
				else if ( i + 1 == argc )
				{
					error(0, 0, "option requires an argument -- '%c'", c);
					fprintf(stderr, "Try '%s --help' for more information\n", argv0);
					exit(1);
				}
				else
				{
					parameter = argv[i+1];
					argv[++i] = NULL;
				}
				errno = 0;
				char* endptr = (char*) parameter;
				display_columns = (size_t) strtoul((char*) parameter, &endptr, 10);
				if ( errno == ERANGE )
					error(1, 0, "option -c `%s' is out of range", parameter);
				if  ( *endptr )
					error(1, 0, "option -c `%s' is invalid", parameter);
			} break;
			case 'e': flag_empty = true; break;
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

	bool success = true;
	struct line* lines = NULL;
	size_t lines_used = 0;
	size_t lines_length = 0;

	for ( int i = 1; i < argc; i++ )
	{
		const char* arg = argv[i];
		FILE* fp = fopen(arg, "r");
		if ( !fp )
		{
			error(0, errno, "`%s'", arg);
			success = false;
			continue;
		}
		if ( !append_lines_from_file(fp, arg, &lines, &lines_used,
		                             &lines_length, flag_empty) )
		{
			fclose(fp);
			success = false;
			continue;
		}
		fclose(fp);
	}

	if ( argc <= 1 )
	{
		if ( !append_lines_from_file(stdin, "<stdin>", &lines, &lines_used,
		                             &lines_length, flag_empty) )
			success = false;
	}

	for ( size_t rows = 1; lines_used != 0; rows++ )
	{
		size_t total_columns_width = 0;
		size_t column_width = 0;
		size_t row = 0;
		size_t columns = 0;
		for ( size_t line_index = 0; line_index < lines_used; line_index++ )
		{
			if ( column_width < lines[line_index].display_width )
				column_width = lines[line_index].display_width;
			if ( ++row == rows || line_index + 1 == lines_used )
			{
				if ( columns != 0 )
					total_columns_width += 2;
				total_columns_width += column_width;
				if ( display_columns < total_columns_width )
					break;
				row = 0;
				column_width = 0;
				columns++;
			}
		}

		if ( display_columns < total_columns_width && columns != 1 )
			continue;

		if ( columns == 1 )
			rows = lines_used;

		size_t* column_widths = (size_t*) calloc(sizeof(size_t), columns);
		if ( !column_widths )
			error(1, errno, "calloc column widths");

		for ( size_t column = 0; column < columns; column++ )
		{
			size_t column_width = 0;
			for ( size_t i = 0; i < rows; i++ )
			{
				size_t line_index = column * rows + i;
				if ( lines_used <= line_index )
					break;
				if ( column_width < lines[line_index].display_width )
					column_width = lines[line_index].display_width;
			}
			column_widths[column] = column_width;
		}

		for ( size_t row = 0; row < rows; row++ )
		{
			for ( size_t column = 0; column < columns; column++ )
			{
				size_t line_index = column * rows + row;
				struct line* line = &lines[line_index];
				if ( lines_used <= line_index )
					break;
				if ( column != 0 )
					printf("  ");
				printf("%s", line->string);
				size_t new_line_index = (column + 1) * rows + row;
				for ( size_t i = line->display_width;
				      i < column_widths[column] && new_line_index < lines_used;
				      i++ )
					putchar(' ');
			}
			printf("\n");
		}

		free(column_widths);

		if ( ferror(stdout) )
			success = false;

		break;
	}

	return success ? 0 : 1;
}
