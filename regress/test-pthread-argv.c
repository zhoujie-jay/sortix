/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2014.

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

    test-pthread-argv.c
    Tests if the argument vector and environment dies with the main thread.

*******************************************************************************/

#include <pthread.h>
#include <string.h>

#include "test.h"

pthread_t main_thread;

char** global_argv;
char** global_envp;
size_t answer = 0;

void* thread_routine(void* ctx)
{
	(void) ctx;

	int errnum;

	if ( (errnum = pthread_join(main_thread, NULL)) )
		test_error(errnum, "pthread_join");

	size_t recount = 0;
	for ( int i = 0; global_argv[i]; i++ )
		recount += strlen(global_argv[i]);
	for ( int i = 0; global_envp[i]; i++ )
		recount += strlen(global_envp[i]);

	test_assert(answer == recount);

	exit(0);
}

int main(int argc, char* argv[], char* envp[])
{
	(void) argc;

	int errnum;

	for ( int i = 0; argv[i]; i++ )
		answer += strlen(argv[i]);
	for ( int i = 0; envp[i]; i++ )
		answer += strlen(envp[i]);

	global_argv = argv;
	global_envp = envp;

	main_thread = pthread_self();

	pthread_t thread;
	if ( (errnum = pthread_create(&thread, NULL, &thread_routine, NULL)) )
		test_error(errnum, "pthread_create");

	pthread_exit(NULL);
}
