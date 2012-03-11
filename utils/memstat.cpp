/*******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011.

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
	if ( (bytes >> 10ULL) & 1023 ) { unit = KIBI; }
	if ( (bytes >> 20ULL) & 1023 ) { unit = MEBI; }
	if ( (bytes >> 30ULL) & 1023 ) { unit = GIBI; }
	if ( (bytes >> 40ULL) & 1023 ) { unit = TEBI; }
	if ( (bytes >> 50ULL) & 1023 ) { unit = PEBI; }
	if ( (bytes >> 60ULL) & 1023 ) { unit = EXBI; }

	switch ( unit )
	{
		case EXBI:
			printf("%u ZiB ", (bytes >> 60ULL) & 1023);
		case PEBI:
			printf("%u PiB ", (bytes >> 50ULL) & 1023);
		case TEBI:
			printf("%u TiB ", (bytes >> 40ULL) & 1023);
		case GIBI:
			printf("%u GiB ", (bytes >> 30ULL) & 1023);
		case MEBI:
			printf("%u MiB ", (bytes >> 20ULL) & 1023);
		case KIBI:
			printf("%u KiB", (bytes >> 10ULL) & 1023);
			break;
		case BYTES:
			printf("%u B", (bytes >> 0ULL) & 1023);
	}
}

int main(int argc, char* argv[])
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
