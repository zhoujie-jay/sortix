#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <error.h>

int usage()
{
	printf("usage: kill [-n] pid ...\n");
	return 0;
}

int main(int argc, char* argv[])
{
	if ( argc < 2 ) { return usage(); }

	int first = 1;
	int signum = SIGTERM;
	if ( argv[1][0] == '-' )
	{
		signum = atoi(argv[first++]);
		if ( argc < 3 ) { return usage(); }
	}

	int result = 0;

	for ( int i = first; i < argc; i++ )
	{
		pid_t pid = atoi(argv[i]);
		if ( kill(pid, signum) )
		{
			error(0, errno, "(%u)", pid);
			result |= 1;
		}
	}

	return result;
}
