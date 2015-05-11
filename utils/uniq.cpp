/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2014.

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

    uniq.cpp
    Report or filter out repeated lines in a file

*******************************************************************************/

#include <errno.h>
#include <error.h>
#include <locale.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TODO: Implement all the features mandated by POSIX.
// TODO: Implement the useful GNU extensions.

char* read_line(FILE* fp, const char* fpname, int delim)
{
	char* line = NULL;
	size_t line_size = 0;
	ssize_t amount = getdelim(&line, &line_size, delim, fp);
	if ( amount < 0 )
	{
		free(line);
		if ( ferror(fp) )
			error(0, errno, "read: `%s'", fpname);
		return NULL;
	}
	if ( amount && (unsigned char) line[amount-1] == (unsigned char) delim )
		line[amount-1] = '\0';
	return line;
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
	fprintf(fp, "Usage: %s [OPTION]... [INPUT [OUTPUT]]\n", argv0);
	fprintf(fp, "Filter adjacent matching lines from INPUT (or standard input),\n");
	fprintf(fp, "writing to OUTPUT (or standard output).\n");
	fprintf(fp, "\n");
	fprintf(fp, "With no options, matching lines are merged to the first occurrence.\n");
	fprintf(fp, "\n");
	fprintf(fp, "Mandatory arguments to long options are mandatory for short options too.\n");
	fprintf(fp, "  -c, --count           prefix lines by the number of occurrences\n");
	fprintf(fp, "  -d, --repeated        only print duplicate lines\n");
#if 0
	fprintf(fp, "  -D, --all-repeated[=delimit-method]  print all duplicate lines\n");
	fprintf(fp, "                        delimit-method={none(default),prepend,separate}\n");
	fprintf(fp, "                        Delimiting is done with blank lines\n");
	fprintf(fp, "  -f, --skip-fields=N   avoid comparing the first N fields\n");
	fprintf(fp, "  -i, --ignore-case     ignore differences in case when comparing\n");
	fprintf(fp, "  -s, --skip-chars=N    avoid comparing the first N characters\n");
#endif
	fprintf(fp, "  -u, --unique          only print unique lines\n");
	fprintf(fp, "  -z, --zero-terminated  end lines with 0 byte, not newline\n");
#if 0
	fprintf(fp, "  -w, --check-chars=N   compare no more than N characters in lines\n");
#endif
	fprintf(fp, "      --help     display this help and exit\n");
	fprintf(fp, "      --version  output version information and exit\n");
	fprintf(fp, "\n");
#if 0
	fprintf(fp, "A field is a run of blanks (usually spaces and/or TABs), then non-blank\n");
	fprintf(fp, "characters.  Fields are skipped before chars.\n");
	fprintf(fp, "\n");
#endif
	fprintf(fp, "Note: 'uniq' does not detect repeated lines unless they are adjacent.\n");
	fprintf(fp, "You may want to sort the input first, or use `sort -u' without `uniq'.\n");
	fprintf(fp, "Also, comparisons honor the rules specified by `LC_COLLATE'.\n");
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

	bool count = false;
	bool delete_singulars = false;
	bool delete_duplicates = false;
	bool zero_terminated = false;

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
			case 'c': count = true; break;
			case 'd': delete_singulars = true; break;
			case 'u': delete_duplicates = true; break;
			case 'z': zero_terminated = true; break;
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
		else if ( !strcmp(arg, "--count") )
			count = true;
		else if ( !strcmp(arg, "--repeated") )
			delete_singulars = true;
		else if ( !strcmp(arg, "--unique") )
			delete_duplicates = true;
		else if ( !strcmp(arg, "--zero-terminated") )
			zero_terminated = true;
		else
		{
			fprintf(stderr, "%s: unknown option: %s\n", argv0, arg);
			help(stderr, argv0);
			exit(1);
		}
	}

	compact_arguments(&argc, &argv);

	if ( 4 <= argc )
	{
		fprintf(stderr, "%s: extra operand `%s'\n", argv0, argv[3]);
		fprintf(stderr, "Try `%s --help' for more information.\n", argv0);
		exit(1);
	}

	const char* inname = "<stdin>";
	const char* outname = "<stdout>";

	if ( 3 <= argc && !freopen(outname = argv[2], "w", stdout) )
		error(1, errno, "`%s'", argv[2]);

	if ( 2 <= argc && !freopen(inname = argv[1], "r", stdin) )
		error(1, errno, "`%s'", argv[1]);

	int delim = zero_terminated ? '\0' : '\n';

	uintmax_t num_repeats = 0;
	char* prev_line = NULL;
	while ( true )
	{
		char* line = read_line(stdin, inname, delim);

		bool first = !prev_line;
		bool different = !first && (!line || strcoll(line, prev_line) != 0);

		if ( delete_singulars && delete_duplicates )
		{
		}
		else if ( delete_singulars )
		{
			if ( different && 1 < num_repeats )
			{
				if ( count )
					printf("%ju ", num_repeats);
				fputs(prev_line, stdout);
				fputc(delim, stdout);
			}
		}
		else if ( delete_duplicates )
		{
			if ( different && num_repeats == 1 )
			{
				if ( count )
					printf("%ju ", num_repeats);
				fputs(prev_line, stdout);
				fputc(delim, stdout);
			}
		}
		else
		{
			if ( count && different )
			{
				printf("%ju ", num_repeats);
				fputs(prev_line, stdout);
				fputc(delim, stdout);
			}
		}

		if ( different )
			num_repeats = 0;

		if ( !line )
			break;

		bool original = first || strcoll(line, prev_line) != 0;
		if ( !count && !delete_singulars && !delete_duplicates && original )
		{
			fputs(line, stdout);
			fputc(delim, stdout);
		}

		free(prev_line);
		prev_line = line;
		num_repeats++;
	}
	free(prev_line);

	if ( fflush(stdout) == EOF || ferror(stdout) )
		return 1;

	return 0;
}
