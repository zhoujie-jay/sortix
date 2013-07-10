/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011.

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

    memstat.cpp
    Prints system memory usage information.

*******************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <string.h>

void printbytes(unsigned long long bytes)
{
	const unsigned BYTES = 0;
	const unsigned KIBI = 1;
	const unsigned MEBI = 2;
	const unsigned GIBI = 3;
	const unsigned TEBI = 4;
	const unsigned PEBI = 5;
	const unsigned EXBI = 6;

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

int main(int /*argc*/, char* /*argv*/[])
{
	size_t memused = 0;
	size_t memtotal = 0;
	if ( memstat(&memused, &memtotal) ) { perror("memstat"); return 1; }

	printf("memory usage: ");
	printbytes(memused);
	printf(" used / ");
	printbytes(memtotal);
	unsigned percent = ((unsigned long long) memused * 100ULL ) / memtotal;
	printf(" total (%u%s)\n", percent, "%");

	return 0;
}
