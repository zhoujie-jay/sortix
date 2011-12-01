#include <stdio.h>

int main(int argc, char* argv[])
{
	printf("Sortix %zu bit\n", sizeof(size_t) * 8UL);
	return 0;
}
