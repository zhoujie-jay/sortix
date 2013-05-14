/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011.

    This program is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
    more details.

    You should have received a copy of the GNU General Public License along with
    this program. If not, see <http://www.gnu.org/licenses/>.

    benchctxswitch.cpp
    Benchmarks the speed of context switches.

*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

static int uptime(uintmax_t* usecs)
{
	struct timespec uptime;
	if ( clock_gettime(CLOCK_BOOT, &uptime) < 0 )
		return -1;
	*usecs = uptime.tv_sec * 1000000ULL + uptime.tv_nsec / 1000ULL;
	return 0;
}

int main(int /*argc*/, char* /*argv*/[])
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
