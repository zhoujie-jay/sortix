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
	if ( (bytes >> 10) & 1023 ) { unit = KIBI; }
	if ( (bytes >> 20) & 1023 ) { unit = MEBI; }
	if ( (bytes >> 30) & 1023 ) { unit = GIBI; }
	if ( (bytes >> 40) & 1023 ) { unit = TEBI; }
	if ( (bytes >> 50) & 1023 ) { unit = PEBI; }
	if ( (bytes >> 60) & 1023 ) { unit = EXBI; }

	switch ( unit )
	{
		case EXBI:
			printf("%u ZiB ", (bytes >> 60) & 1023);
		case PEBI:
			printf("%u PiB ", (bytes >> 50) & 1023);
		case TEBI:
			printf("%u TiB ", (bytes >> 40) & 1023);
		case GIBI:
			printf("%u GiB ", (bytes >> 30) & 1023);
		case MEBI:
			printf("%u MiB ", (bytes >> 20) & 1023);
		case KIBI:
			printf("%u KiB", (bytes >> 10) & 1023);
			break;
		case BYTES:
			printf("%u B", (bytes >> 0) & 1023);
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
