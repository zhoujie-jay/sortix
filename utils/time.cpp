/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

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

    time.cpp
    Measure process running time.

*******************************************************************************/

#include <sys/types.h>
#include <sys/wait.h>

#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <timespec.h>
#include <unistd.h>

int main(int argc, char* argv[])
{
	struct timespec start_time;
	clock_gettime(CLOCK_MONOTONIC, &start_time);
	pid_t child_pid = fork();
	if ( child_pid < 0 )
		error(1, errno, "fork");
	if ( !child_pid )
	{
		if ( argc <= 1 )
			exit(0);
		execvp(argv[1], argv+1);
		error(127, errno, "%s", argv[1]);
	}
	int exit_status = 0;
	waitpid(child_pid, &exit_status, 0);

	struct timespec end_time;
	clock_gettime(CLOCK_MONOTONIC, &end_time);
	struct tmns tmns;
	timens(&tmns);
	struct timespec real_time = timespec_sub(end_time, start_time);
	struct timespec execute_time = timespec_sub(tmns.tmns_cutime, tmns.tmns_utime);
	struct timespec system_time = timespec_sub(tmns.tmns_cstime, tmns.tmns_stime);
	struct timespec user_time = timespec_sub(execute_time, system_time);

	fprintf(stderr, "\n");
	fprintf(stderr, "real\t%lim%li.%03li\n", (long) real_time.tv_sec / 60, (long) real_time.tv_sec % 60, real_time.tv_nsec / 1000000L);
	fprintf(stderr, "user\t%lim%li.%03li\n", (long) user_time.tv_sec / 60, (long) user_time.tv_sec % 60, user_time.tv_nsec / 1000000);
	fprintf(stderr, "sys\t%lim%li.%03li\n", (long) system_time.tv_sec / 60, (long) system_time.tv_sec % 60, system_time.tv_nsec / 1000000);

	return WEXITSTATUS(exit_status);
}
