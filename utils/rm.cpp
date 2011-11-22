#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

int main(int argc, char* argv[])
{
	if ( argc < 2 ) { printf("usage: %s <file> ...\n"); return 0; }

	int result = 0;

	for ( int i = 1; i < argc; i++ )
	{
		if ( unlink(argv[i]) )
		{
			printf("%s: cannot remove %s: %s\n", argv[0], argv[i], strerror(errno));
			result = 1;
		}
	}

	return result;
}
