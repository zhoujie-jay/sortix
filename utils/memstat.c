/*
 * Copyright (c) 2011 Jonas 'Sortie' Termansen.
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
 * memstat.c
 * Prints system memory usage information.
 */

#include <err.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define BYTES 0
#define KIBI 1
#define MEBI 2
#define GIBI 3
#define TEBI 4
#define PEBI 5
#define EXBI 6

void printbytes(unsigned long long bytes)
{
	unsigned unit = BYTES;
	if ( (bytes >> 10ULL) & 1023ULL ) { unit = KIBI; }
	if ( (bytes >> 20ULL) & 1023ULL ) { unit = MEBI; }
	if ( (bytes >> 30ULL) & 1023ULL ) { unit = GIBI; }
	if ( (bytes >> 40ULL) & 1023ULL ) { unit = TEBI; }
	if ( (bytes >> 50ULL) & 1023ULL ) { unit = PEBI; }
	if ( (bytes >> 60ULL) & 1023ULL ) { unit = EXBI; }

	switch ( unit )
	{
		case EXBI:
			printf("%llu ZiB ", (bytes >> 60ULL) & 1023ULL);
		case PEBI:
			printf("%llu PiB ", (bytes >> 50ULL) & 1023ULL);
		case TEBI:
			printf("%llu TiB ", (bytes >> 40ULL) & 1023ULL);
		case GIBI:
			printf("%llu GiB ", (bytes >> 30ULL) & 1023ULL);
		case MEBI:
			printf("%llu MiB ", (bytes >> 20ULL) & 1023ULL);
		case KIBI:
			printf("%llu KiB", (bytes >> 10ULL) & 1023ULL);
			break;
		case BYTES:
			printf("%llu B", (bytes >> 0ULL) & 1023ULL);
	}
}

int main(void)
{
	size_t memused = 0;
	size_t memtotal = 0;
	if ( memstat(&memused, &memtotal) )
		err(1, "memstat");

	printf("memory usage: ");
	printbytes(memused);
	printf(" used / ");
	printbytes(memtotal);
	unsigned percent = ((unsigned long long) memused * 100ULL ) / memtotal;
	printf(" total (%u%s)\n", percent, "%");

	return 0;
}
