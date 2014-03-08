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

    sort.cpp
    Sort, merge, or sequence check text files.

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

int flip_comparison(int rel)
{
	return rel < 0 ? 1 : 0 < rel ? -1 : 0;
}

int indirect_compare(int (*compare)(const char*, const char*),
                     const void* a_ptr, const void* b_ptr)
{
	const char* a = *(const char* const*) a_ptr;
	const char* b = *(const char* const*) b_ptr;
	return compare(a, b);
}

int compare_line(const char* a, const char* b)
{
	return strcoll(a, b);
}

int indirect_compare_line(const void* a_ptr, const void* b_ptr)
{
	return indirect_compare(compare_line, a_ptr, b_ptr);
}

int compare_line_reverse(const char* a, const char* b)
{
	return flip_comparison(compare_line(a, b));
}

int indirect_compare_line_reverse(const void* a_ptr, const void* b_ptr)
{
	return indirect_compare(compare_line, a_ptr, b_ptr);
}

int compare_version(const char* a, const char* b)
{
	return strverscmp(a, b);
}

int indirect_compare_version(const void* a_ptr, const void* b_ptr)
{
	return indirect_compare(compare_version, a_ptr, b_ptr);
}

int compare_version_reverse(const char* a, const char* b)
{
	return flip_comparison(compare_version(a, b));
}

int indirect_compare_version_reverse(const void* a_ptr, const void* b_ptr)
{
	return indirect_compare(compare_version, a_ptr, b_ptr);
}

struct input_stream
{
	const char* const* files;
	size_t files_current;
	size_t files_length;
	FILE* current_file;
	const char* last_file_path;
	uintmax_t last_line_number;
	bool result_status;
};

char* read_line(FILE* fp, const char* fpname, int delim)
{
	char* line = NULL;
	size_t line_size = 0;
	ssize_t amount = getdelim(&line, &line_size, delim, fp);
	if ( amount < 0 )
	{
		if ( ferror(fp) )
			error(0, errno, "read: `%s'", fpname);
		return NULL;
	}
	if ( amount && (unsigned char) line[amount-1] == (unsigned char) delim )
		line[amount-1] = '\0';
	return line;
}

char* read_input_stream_line(struct input_stream* is, int delim)
{
	if ( !is->files_length )
	{
		char* result = read_line(stdin, "<stdin>", delim);
		if ( ferror(stdin) )
			is->result_status = false;
		is->last_file_path = "-";
		is->last_line_number++;
		return result;
	}
	while ( is->files_current < is->files_length )
	{
		const char* path = is->files[is->files_current];
		if ( !is->current_file )
		{
			is->last_line_number = 0;
			if ( !strcmp(path, "-") )
				is->current_file = stdin;
			else if ( !(is->current_file = fopen(path, "r")) )
			{
				error(0, errno, "`%s'", path);
				is->result_status = false;
				is->files_current++;
				continue;
			}
		}
		char* result = read_line(is->current_file, path, delim);
		if ( !result )
		{
			if ( ferror(is->current_file) )
			{
				error(0, errno, "reading: `%s'", path);
				is->result_status = false;
			}
			if ( is->current_file != stdin )
				fclose(is->current_file);
			is->current_file = NULL;
			is->files_current++;
			continue;
		}
		is->last_file_path = path;
		is->last_line_number++;
		return result;
	}
	return NULL;
}

char** read_input_stream_lines(size_t* result_num_lines,
                               struct input_stream* is,
                               int delim)
{
	char** lines = NULL;
	size_t lines_used = 0;
	size_t lines_length = 0;

	while ( char* line = read_input_stream_line(is, delim) )
	{
		if ( lines_used == lines_length )
		{
			size_t new_lines_length = lines_length ? 2 * lines_length : 64;
			size_t new_lines_size = sizeof(char*) * new_lines_length;
			char** new_lines = (char**) realloc(lines, new_lines_size);
			if ( !new_lines )
			{
				error(0, errno, "realloc");
				free(line);
				is->result_status = false;
				return *result_num_lines = lines_used, lines;
			}
			lines = new_lines;
			lines_length = new_lines_length;
		}
		lines[lines_used++] = line;
	}

	return *result_num_lines = lines_used, lines;
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
	fprintf(fp, "Write sorted concatenation of all FILE(s) to standard output.\n");
	fprintf(fp, "\n");
	fprintf(fp, "Mandatory arguments to long options are mandatory for short options too.\n");
	fprintf(fp, "Ordering options:\n");
	fprintf(fp, "\n");
#if 0
	fprintf(fp, "  -b, --ignore-leading-blanks  ignore leading blanks\n");
	fprintf(fp, "  -d, --dictionary-order      consider only blanks and alphanumeric characters\n");
	fprintf(fp, "  -f, --ignore-case           fold lower case to upper case characters\n");
	fprintf(fp, "  -g, --general-numeric-sort  compare according to general numerical value\n");
	fprintf(fp, "  -i, --ignore-nonprinting    consider only printable characters\n");
	fprintf(fp, "  -M, --month-sort            compare (unknown) < `JAN' < ... < `DEC'\n");
	fprintf(fp, "  -h, --human-numeric-sort    compare human readable numbers (e.g., 2K 1G)\n");
	fprintf(fp, "  -n, --numeric-sort          compare according to string numerical value\n");
	fprintf(fp, "  -R, --random-sort           sort by random hash of keys\n");
	fprintf(fp, "      --random-source=FILE    get random bytes from FILE\n");
#endif
	fprintf(fp, "  -r, --reverse               reverse the result of comparisons\n");
#if 0
	fprintf(fp, "      --sort=WORD             sort according to WORD:\n");
	fprintf(fp, "                                general-numeric -g, human-numeric -h, month -M,\n");
	fprintf(fp, "                                numeric -n, random -R, version -V\n");
#endif
	fprintf(fp, "  -V, --version-sort          natural sort of (version) numbers within text\n");
	fprintf(fp, "\n");
	fprintf(fp, "Other options:\n");
	fprintf(fp, "\n");
#if 0
	fprintf(fp, "      --batch-size=NMERGE   merge at most NMERGE inputs at once;\n");
	fprintf(fp, "                            for more use temp files\n");
#endif
	fprintf(fp, "  -c, --check, --check=diagnose-first  check for sorted input; do not sort\n");
	fprintf(fp, "  -C, --check=quiet, --check=silent  like -c, but do not report first bad line\n");
#if 0
	fprintf(fp, "      --compress-program=PROG  compress temporaries with PROG;\n");
	fprintf(fp, "                              decompress them with PROG -d\n");
	fprintf(fp, "      --debug               annotate the part of the line used to sort,\n");
	fprintf(fp, "                              and warn about questionable usage to stderr\n");
	fprintf(fp, "      --files0-from=F       read input from the files specified by\n");
	fprintf(fp, "                            NUL-terminated names in file F;\n");
	fprintf(fp, "                            If F is - then read names from standard input\n");
	fprintf(fp, "  -k, --key=POS1[,POS2]     start a key at POS1 (origin 1), end it at POS2\n");
	fprintf(fp, "                            (default end of line).  See POS syntax below\n");
#endif
	fprintf(fp, "  -m, --merge               merge already sorted files; do not sort\n");
	fprintf(fp, "  -o, --output=FILE         write result to FILE instead of standard output\n");
#if 0
	fprintf(fp, "  -s, --stable              stabilize sort by disabling last-resort comparison\n");
	fprintf(fp, "  -S, --buffer-size=SIZE    use SIZE for main memory buffer\n");
	fprintf(fp, "  -t, --field-separator=SEP  use SEP instead of non-blank to blank transition\n");
	fprintf(fp, "  -T, --temporary-directory=DIR  use DIR for temporaries, not $TMPDIR or /tmp;\n");
	fprintf(fp, "                              multiple options specify multiple directories\n");
	fprintf(fp, "      --parallel=N          change the number of sorts run concurrently to N\n");
#endif
	fprintf(fp, "  -u, --unique              with -c, check for strict ordering;\n");
	fprintf(fp, "                              without -c, output only the first of an equal run\n");
	fprintf(fp, "  -z, --zero-terminated     end lines with 0 byte, not newline\n");
	fprintf(fp, "      --help                display this help and exit\n");
	fprintf(fp, "      --version             output version information and exit\n");
	fprintf(fp, "\n");
#if 0
	fprintf(fp, "POS is F[.C][OPTS], where F is the field number and C the character position\n");
	fprintf(fp, "in the field; both are origin 1.  If neither -t nor -b is in effect, characters\n");
	fprintf(fp, "in a field are counted from the beginning of the preceding whitespace.  OPTS is\n");
	fprintf(fp, "one or more single-letter ordering options, which override global ordering\n");
	fprintf(fp, "options for that key.  If no key is given, use the entire line as the key.\n");
	fprintf(fp, "\n");
	fprintf(fp, "SIZE may be followed by the following multiplicative suffixes:\n");
	fprintf(fp, "%% 1%% of memory, b 1, K 1024 (default), and so on for M, G, T, P, E, Z, Y.\n");
	fprintf(fp, "\n");
#endif
	fprintf(fp, "With no FILE, or when FILE is -, read standard input.\n");
	fprintf(fp, "\n");
	fprintf(fp, "*** WARNING ***\n");
	fprintf(fp, "The locale specified by the environment affects sort order.\n");
	fprintf(fp, "Set LC_ALL=C to get the traditional sort order that uses\n");
	fprintf(fp, "native byte values.\n");
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

	bool check = false;
	bool check_quiet = false;
	bool merge = false;
	const char* output = NULL;
	bool reverse = false;
	bool unique = false;
	bool version_sort = false;
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
			case 'c': check = true; break;
			case 'C': check = check_quiet = false; break;
			case 'm': merge = true; break;
			case 'o':
				if ( !*(output = arg + 1) )
				{
					if ( i + 1 == argc )
					{
						error(0, 0, "option requires an argument -- 'o'");
						fprintf(stderr, "Try `%s --help' for more information.\n", argv[0]);
						exit(125);
					}
					output = argv[i+1];
					argv[++i] = NULL;
				}
				arg = "o";
				break;
			case 'r': reverse = true; break;
			case 'u': unique = true; break;
			case 'V': version_sort = true; break;
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
		else if ( !strcmp(arg, "--check") ||
		          !strcmp(arg, "--check=diagnose-first") )
			check = true, check_quiet = false;
		else if ( !strcmp(arg, "--check=quiet") ||
		          !strcmp(arg, "--check=silent") )
			check = true, check_quiet = true;
		else if ( !strcmp(arg, "--merge") )
			merge = true;
		else if ( !strncmp(arg, "--output=", strlen("--output=")) )
			output = arg + strlen("--output=");
		else if ( !strcmp(arg, "--output") )
		{
			if ( i + 1 == argc )
			{
				error(0, 0, "option '--output' requires an argument");
				fprintf(stderr, "Try `%s --help' for more information.\n", argv[0]);
				exit(125);
			}
			output = argv[i+1];
			argv[++i] = NULL;
		}
		else if ( !strcmp(arg, "--reverse") )
			reverse = true;
		else if ( !strcmp(arg, "--unique") )
			unique = true;
		else if ( !strcmp(arg, "--version-sort") )
			version_sort = true;
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

	if ( output && !freopen(output, "w", stdout) )
		error(2, errno, "`%s'", output);

	int delim = zero_terminated ? '\0' : '\n';

	int (*compare)(const char*, const char*);
	int (*qsort_compare)(const void*, const void*);

	if ( version_sort && reverse )
		compare = compare_version_reverse,
		qsort_compare = indirect_compare_version_reverse;
	else if ( version_sort )
		compare = compare_version,
		qsort_compare = indirect_compare_version;
	else if ( reverse )
		compare = compare_line_reverse,
		qsort_compare = indirect_compare_line_reverse;
	else
		compare = compare_line,
		qsort_compare = indirect_compare_line;

	struct input_stream is;
	memset(&is, 0, sizeof(is));
	is.files = argv + 1;
	is.files_current = 0;
	is.files_length = argc - 1;
	is.result_status = true;

	if ( check )
	{
		int needed_relation = unique ? 1 : 0;
		char* prev_line = NULL;
		while ( char* line = read_input_stream_line(&is, delim) )
		{
			if ( prev_line && compare(line, prev_line) < needed_relation )
			{
				if ( !check_quiet )
					error(0, errno, "%s:%ju: disorder: %s", is.last_file_path,
					                is.last_line_number, line);
				exit(1);
			}
			free(prev_line);
			prev_line = line;
		}
		free(prev_line);
	}
	else
	{
		(void) merge;

		size_t lines_used = 0;
		char** lines = read_input_stream_lines(&lines_used, &is, delim);

		qsort(lines, lines_used, sizeof(*lines), qsort_compare);

		for ( size_t i = 0; i < lines_used; i++ )
		{
			if ( unique && i && compare(lines[i-1], lines[i]) == 0 )
				continue;
			fputs(lines[i], stdout);
			fputc(delim, stdout);
		}
	}

	return is.result_status ? 0 : 2;
}
