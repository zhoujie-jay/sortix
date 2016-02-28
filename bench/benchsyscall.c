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

    benchsyscall.c
    Benchmarks the speed of system calls.

*******************************************************************************/

#include <err.h>
#include <stdio.h>
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

int main(void)
{
	uintmax_t start;
	if ( uptime(&start) )
		err(1, "uptime");
	uintmax_t end = start + 1ULL * 1000ULL * 1000ULL; // 1 second
	size_t count = 0;
	uintmax_t now;
	while ( !uptime(&now) && now < end ) { count++; }
	printf("Made %zu system calls in 1 second\n", count);
	return 0;
}
