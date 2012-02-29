#include <stdio.h>
#include <unistd.h>

int main(int argc, char* argv[])
{
	const size_t WDSIZE = 4096;
	char wd[WDSIZE];
	if ( !getcwd(wd, WDSIZE) ) { perror("getcwd"); return 1; }
	printf("%s\n", wd);
	return 0;
}

