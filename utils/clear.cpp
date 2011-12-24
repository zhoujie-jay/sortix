#include <stdio.h>

int main(int argc, char* argv[])
{
	printf("\e[H\e[2J");
	fflush(stdout);
	return 0;
}
