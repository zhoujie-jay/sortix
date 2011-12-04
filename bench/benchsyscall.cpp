#include <stdio.h>
#include <unistd.h>

int main(int argc, char* argv[])
{
	uintmax_t start;
	if ( uptime(&start) ) { perror("uptime"); return 1; }
	uintmax_t end = start + 1ULL * 1000ULL * 1000ULL; // 1 second
	size_t count = 0;
	uintmax_t now;
	while ( !uptime(&now) && now < end ) { count++; }
	printf("Made %zu system calls in 1 second\n", count);
	return 0;
}
