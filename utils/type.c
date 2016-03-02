/*
 * Copyright (c) 2011, 2012 Jonas 'Sortie' Termansen.
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
 * type.c
 * Lets you move the tty cursor around and easily issue ANSI escape codes.
 */

#include <sys/keycodes.h>
#include <sys/termmode.h>
#include <errno.h>
#include <error.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void help(FILE* fp, const char* argv0)
{
	fprintf(fp, "Usage: %s [OPTION]...\n", argv0);
	fprintf(fp, "Lets you type freely onto the tty.\n");
}

static void version(FILE* fp, const char* argv0)
{
	fprintf(fp, "%s (Sortix) %s\n", argv0, VERSIONSTR);
}

int main(int argc, char* argv[])
{
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

	if ( !isatty(0) || !isatty(1) )
	{
		error(1, errno, "standard output and input must be a tty");
	}

	bool lastwasesc = false;
	unsigned termmode = TERMMODE_KBKEY | TERMMODE_UNICODE | TERMMODE_SIGNAL;
	if ( settermmode(0, termmode) ) { error(1, errno, "settermmode"); }
	while ( true )
	{
		uint32_t codepoint;
		ssize_t numbytes = read(0, &codepoint, sizeof(codepoint));
		if ( !numbytes ) { break; }
		if ( numbytes < 0 ) { break; }
		int kbkey = KBKEY_DECODE(codepoint);
		if ( kbkey < 0 ) { continue; }
		if ( kbkey == KBKEY_UP ) { printf("\e[A"); fflush(stdout); continue; }
		if ( kbkey == KBKEY_DOWN ) { printf("\e[B"); fflush(stdout); continue; }
		if ( kbkey == KBKEY_RIGHT ) { printf("\e[C"); fflush(stdout); continue; }
		if ( kbkey == KBKEY_LEFT ) { printf("\e[D"); fflush(stdout); continue; }
		if ( kbkey == KBKEY_ESC ) { printf("\e["); fflush(stdout); lastwasesc = true; continue; }
		if ( kbkey ) { continue; }
		if ( lastwasesc && codepoint == '[' ) { continue; }

		if ( codepoint >= 0x80 ) { continue; }

		putchar(codepoint & 0xFF);
		lastwasesc = false;
		fflush(stdout);
	}

	return 0;
}
