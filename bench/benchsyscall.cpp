/*******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011.

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

	benchsyscall.cpp
	Benchmarks the speed of system calls.

*******************************************************************************/

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
