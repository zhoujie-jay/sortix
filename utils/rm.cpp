#include <stdio.h>
#include <unistd.h>

int main(int argc, char* argv[])
{
	if ( argc < 2 ) { printf("usage: %s <file> ...\n"); return 0; }

	int result = 0;

	for ( int i = 1; i < argc; i++ )
	{
		if ( unlink(argv[i]) )
		{
			printf("%s: unable to unlink: %s\n", argv[0], argv[i]);
			result = 1;
		}
	}

	return result;
}
