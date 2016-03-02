/*
 * Copyright (c) 2013 Jonas 'Sortie' Termansen.
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
 * wc.c
 * Counts bytes, characters, words and lines.
 */

#include <sys/stat.h>
#include <sys/types.h>

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <error.h>
#include <locale.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>

#define FLAG_PRINT_NUM_BYTES (1 << 0)
#define FLAG_PRINT_NUM_CHARACTERS (1 << 1)
#define FLAG_PRINT_NUM_WORDS (1 << 2)
#define FLAG_PRINT_NUM_LINES (1 << 3)
#define FLAG_PRINT_COMPACT (1 << 4)

#define DEFAULT_FLAGS (FLAG_PRINT_NUM_BYTES | FLAG_PRINT_NUM_WORDS | FLAG_PRINT_NUM_LINES)

struct word_count
{
	uintmax_t num_bytes;
	uintmax_t num_characters;
	uintmax_t num_words;
	uintmax_t num_lines;
};

static struct word_count count_words(FILE* fp)
{
	struct word_count stats;
	memset(&stats, 0, sizeof(stats));

	mbstate_t mbstate;
	memset(&mbstate, 0, sizeof(mbstate));

	bool word_begun = false;
	bool line_begun = false;

	int ic;
	while ( (ic = fgetc(fp)) != EOF )
	{
		stats.num_bytes++;

		char c = (char) ((unsigned char) ic);

		wchar_t wc;
		size_t num_converted = mbrtowc(&wc, &c, 1, &mbstate);
		if ( num_converted == (size_t) -1 )
		{
			memset(&mbstate, 0, sizeof(mbstate));
			continue;
		}
		if ( num_converted == (size_t) -2 )
			continue;
		// TODO: Is this strictly speaking needed?
		if ( !num_converted )
			wc = L'\0';

		stats.num_characters++;
		word_begun = !iswspace(wc) ||
		             (word_begun ? (stats.num_words++, false) : false);
		line_begun = wc != L'\n' || (stats.num_lines++, false);
	}

	if ( word_begun )
		stats.num_words++;
	if ( line_begun )
		stats.num_lines++;

	return stats;
}

static void print_stat(FILE* fp, uintmax_t value, int flags, int cond)
{
	if ( !(flags & cond) )
		return;
	if ( flags & FLAG_PRINT_COMPACT )
	{
		fprintf(fp, "%ju", value);
		return;
	}
	if ( value < 100000 )
	{
		fprintf(fp, "%6ju", value);
		return;
	}
	fprintf(fp, " %ju ", value);
}

static
void print_stats(struct word_count stats, FILE* fp, int flags, const char* path)
{
	// TODO: Proper columnization of large values will require knowing all the
	//       row values in advance - so we'll have to remember the statistics
	//       for every file we process before printing!
	print_stat(fp, stats.num_lines, flags, FLAG_PRINT_NUM_LINES);
	print_stat(fp, stats.num_words, flags, FLAG_PRINT_NUM_WORDS);
	print_stat(fp, stats.num_bytes, flags, FLAG_PRINT_NUM_BYTES);
	print_stat(fp, stats.num_characters, flags, FLAG_PRINT_NUM_CHARACTERS);
	if ( path )
		fprintf(fp, " %s", path);
	fprintf(fp, "\n");
}

static void help(FILE* fp, const char* argv0)
{
	fprintf(fp, "Usage: %s [OPTION]...\n", argv0);
	fprintf(fp, "Print newline, word, and byte counts for each FILE, and a total line if\n");
	fprintf(fp, "more than one FILE is specified.  With no FILE, or when FILE is -,\n");
	fprintf(fp, "read standard input.  A word is a non-zero-length sequence of characters\n");
	fprintf(fp, "delimited by white space.\n");
	fprintf(fp, "The options below may be used to select which counts are printed, always in\n");
	fprintf(fp, "the following order: newline, word, character, byte.\n");
	fprintf(fp, "\n");
	fprintf(fp, "  -c, --bytes     print the byte counts\n");
	fprintf(fp, "  -m, --chars     print the character counts\n");
	fprintf(fp, "  -l, --lines     print the newline counts\n");
	fprintf(fp, "  -w, --words     print the word counts\n");
	fprintf(fp, "      --help      display this help and exit\n");
	fprintf(fp, "      --version   output version information and exit\n");
}

static void version(FILE* fp, const char* argv0)
{
	fprintf(fp, "%s (Sortix) %s\n", argv0, VERSIONSTR);
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

bool word_count_file(FILE* fp, const char* path, int flags,
                     struct word_count* total)
{
	struct stat st;
	if ( fstat(fileno(fp), &st) == 0 && S_ISDIR(st.st_mode) )
	{
		struct word_count word_count;
		memset(&word_count, 0, sizeof(word_count));
		error(0, EISDIR, "`%s'", path);
		print_stats(word_count, stdout, flags, path);
		return false;
	}
	struct word_count word_count = count_words(fp);
	// TODO: Possible overflow here!
	if ( total )
	{
		total->num_bytes += word_count.num_bytes;
		total->num_characters += word_count.num_characters;
		total->num_words += word_count.num_words;
		total->num_lines += word_count.num_lines;
	}
	if ( ferror(fp) )
	{
		error(0, errno, "`%s'", path);
		print_stats(word_count, stdout, flags, path);
		return false;
	}
	print_stats(word_count, stdout, flags, path);
	return true;
}

int word_count_files(int argc, char* argv[], int flags)
{
	if ( argc <= 1 )
		return word_count_file(stdin, NULL, flags, NULL);

	struct word_count total_count;
	memset(&total_count, 0, sizeof(total_count));

	bool success = true;
	for ( int i = 1; i < argc; i++ )
	{
		if ( !strcmp(argv[i], "-") )
		{
			if ( !word_count_file(stdin, "-", flags, NULL) )
				success = false;
			continue;
		}

		FILE* fp = fopen(argv[i], "r");
		if ( !fp )
		{
			error(0, errno, "`%s'", argv[i]);
			struct word_count word_count;
			memset(&word_count, 0, sizeof(word_count));
			print_stats(word_count, stdout, flags, argv[i]);
			success = false;
			continue;
		}

		if ( !word_count_file(fp, argv[i], flags, &total_count) )
			success = false;

		fclose(fp);
	}

	if ( 3 <= argc )
		print_stats(total_count, stdout, flags, "total");

	return success;
}

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "");

	int flags = 0;

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
			char c;
			while ( (c = *++arg) ) switch ( c )
			{
			case 'c': flags |= FLAG_PRINT_NUM_BYTES; break;
			case 'l': flags |= FLAG_PRINT_NUM_LINES; break;
			case 'm': flags |= FLAG_PRINT_NUM_CHARACTERS; break;
			case 'w': flags |= FLAG_PRINT_NUM_WORDS; break;
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
		else if ( !strcmp(arg, "--bytes") )
			flags |= FLAG_PRINT_NUM_BYTES;
		else if ( !strcmp(arg, "--chars") )
			flags |= FLAG_PRINT_NUM_CHARACTERS;
		else if ( !strcmp(arg, "--lines") )
			flags |= FLAG_PRINT_NUM_LINES;
		else if ( !strcmp(arg, "--words") )
			flags |= FLAG_PRINT_NUM_WORDS;
		else
		{
			fprintf(stderr, "%s: unknown option: %s\n", argv0, arg);
			help(stderr, argv0);
			exit(1);
		}
	}

	compact_arguments(&argc, &argv);

	if ( !flags )
		flags = DEFAULT_FLAGS;

	if ( flags && flags == 1 << (ffs(flags)-1) && argc <= 2 )
		flags |= FLAG_PRINT_COMPACT;

	return word_count_files(argc, argv, flags) ? 0 : 1;
}
