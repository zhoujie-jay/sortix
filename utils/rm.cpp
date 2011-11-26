#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <error.h>

int main(int argc, char* argv[])
{
	if ( argc < 2 ) { printf("usage: %s <file> ...\n", argv[0]); return 0; }

	int result = 0;

	for ( int i = 1; i < argc; i++ )
	{
		if ( unlink(argv[i]) )
		{
			error(0, errno, "cannot remove %s", argv[i]);
			result = 1;
		}
	}

	return result;
}
