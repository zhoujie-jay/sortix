/*
 * Copyright (c) 2014 Jonas 'Sortie' Termansen.
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
 * sleep.c
 * Suspend execution for an interval of time.
 */

#include <ctype.h>
#include <errno.h>
#include <error.h>
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <timespec.h>

bool is_valid_interval(const char* string)
{
	size_t index = 0;
	while ( '0' <= string[index] && string[index] <= '9' )
		index++;
	if ( string[index] == '.' )
	{
		index++;
		while ( '0' <= string[index] && string[index] <= '9' )
			index++;
		if ( index == 1 && string[index-1] == '.' )
			return false;
	}
	if ( index == 0 )
		return false;
	return strcmp(string + index, "ns") == 0 ||
	       strcmp(string + index, "us") == 0 ||
	       strcmp(string + index, "ms") == 0 ||
	       strcmp(string + index, "") == 0 ||
	       strcmp(string + index, "s") == 0 ||
	       strcmp(string + index, "m") == 0 ||
	       strcmp(string + index, "h") == 0 ||
	       strcmp(string + index, "d") == 0;
}

struct timespec parse_interval(const char* string)
{
	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = 0;
	size_t index = 0;

	while ( '0' <= string[index] && string[index] <= '9' )
	{
		char c = string[index++];
		// TODO: Prevent the multiplication from overflowing!
		ts.tv_sec = ts.tv_sec * 10;
		// TODO: Prevent the addition from overflowing!
		ts.tv_sec += c - '0';
	}

	if ( string[index] == '.' )
	{
		index++;

		// Pick 9 digits as nanoseconds.
		long contribution = 100L * 1000L * 1000L;
		while ( contribution && '0' <= string[index] && string[index] <= '9' )
		{
			char c = string[index++];
			ts.tv_nsec += (c - '0') * contribution;
			contribution /= 10;
		}

		// Remember whether there are more non-zero digits we didn't handle.
		bool any_non_zero_digits = false;
		for ( size_t i = index; string[i]; i++ )
		{
			if ( !('0' <= string[i] && string[i] <= '9') )
				break;
			if ( string[i] - '0' != 0 )
				any_non_zero_digits = true;
		}

		// If there are more digits we didn't handle, we'll round based on that.
		if ( '0' <= string[index] && string[index] <= '9' )
		{
			char c = string[index++];
			if ( 5 <= c - '0' )
				ts.tv_nsec++;
			if ( ts.tv_nsec == 1000000000L )
			{
				ts.tv_nsec = 0;
				// TODO: This could overflow, which case we shouldn't have added
				//       1 to tv_nsec regardless.
				ts.tv_sec++;
			}
		}

		// If all the digits we handled were zeroes, but there were some obscure
		// non-zero digits we didn't handle, we'll wait at least a nanosecond.
		if ( ts.tv_sec == 0 && ts.tv_nsec == 0 && any_non_zero_digits )
			ts.tv_nsec = 1;
	}

	if ( !strcmp(string + index, "ns") )
	{
		ts.tv_nsec = ts.tv_sec % 1000000000L;
		ts.tv_sec /= 1000000000L;
	}

	if ( !strcmp(string + index, "us") )
	{
		ts.tv_nsec /= 1000000L;
		ts.tv_nsec += (ts.tv_sec % 1000000L) * 1000L;
		ts.tv_sec /= 1000000L;
	}

	if ( !strcmp(string + index, "ms") )
	{
		ts.tv_nsec /= 1000L;
		ts.tv_nsec += (ts.tv_sec % 1000L) * 100000L;
		ts.tv_sec /= 1000L;
	}

	if ( !strcmp(string + index, "") || !strcmp(string + index, "s") )
	{
	}

	if ( !strcmp(string + index, "m") )
	{
		// TODO: Properly handle overflow here!
		ts.tv_nsec *= 60;
		ts.tv_sec *= 60;
		ts = timespec_canonalize(ts);
	}

	if ( !strcmp(string + index, "h") )
	{
		// TODO: Properly handle overflow here!
		ts.tv_nsec *= 3600;
		ts.tv_sec *= 3600;
		ts = timespec_canonalize(ts);
	}

	if ( !strcmp(string + index, "d") )
	{
		// TODO: Properly handle overflow here!
		ts.tv_nsec *= 86400;
		ts.tv_sec *= 86400;
		ts = timespec_canonalize(ts);
	}

	return ts;
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
	fprintf(fp, "Usage: %s [OPTION]... NUMBER[SUFFIX]...\n", argv0);
	fprintf(fp, "Pause for NUMBER seconds.  SUFFIX may be 's' for seconds (the default),\n");
	fprintf(fp, "'m' for minutes, 'h' for hours or 'd' for days.  Unlike most implementations\n");
	fprintf(fp, "that require NUMBER be an integer, here NUMBER may be an arbitrary floating\n");
	fprintf(fp, "point number.  Given two or more arguments, pause for the amount of time\n");
	fprintf(fp, "specified by the sum of their values.\n");
	fprintf(fp, "\n");
	fprintf(fp, "      --help     display this help and exit\n");
	fprintf(fp, "      --version  output version information and exit\n");
}

static void version(FILE* fp, const char* argv0)
{
	fprintf(fp, "%s (Sortix) %s\n", argv0, VERSIONSTR);
}

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "");

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

	if ( argc < 2 )
	{
		error(0, errno, "missing operand");
		fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
		exit(1);
	}

	struct timespec ts = timespec_nul();

	for ( int i = 1; i < argc; i++ )
	{
		const char* arg = argv[i];
		if ( !is_valid_interval(arg) )
		{
			error(0, errno, "invalid sleep interval `%s'", arg);
			fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
			exit(1);
		}
		ts = timespec_add(ts, parse_interval(arg));
	}

	while ( ts.tv_sec || ts.tv_nsec )
	{
		if ( clock_nanosleep(CLOCK_REALTIME, 0, &ts, &ts) != 0 )
			error(1, errno, "clock_nanosleep");
	}

	return 0;
}
