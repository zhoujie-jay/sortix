#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

int main(int argc, char* argv[])
{
	pid_t slavepid = fork();
	if ( slavepid < 0 ) { perror("fork"); return 1; }
	if ( slavepid == 0 ) { while ( true ) { usleep(0); } exit(0); }

	uintmax_t start;
	if ( uptime(&start) ) { perror("uptime"); return 1; }
	uintmax_t end = start + 1ULL * 1000ULL * 1000ULL; // 1 second
	size_t count = 0;
	uintmax_t now;
	while ( !uptime(&now) && now < end ) { usleep(0); count += 2; /* back and forth */ }
	printf("Made %zu context switches in 1 second\n", count);

	kill(slavepid, SIGKILL);

	return 0;
}
