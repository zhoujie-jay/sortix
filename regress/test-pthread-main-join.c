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

    test-pthread-main-join.c
    Tests whether the main thread can be joined.

*******************************************************************************/

#include <pthread.h>

#include "test.h"

pthread_t main_thread;

void* thread_routine(void* expected_result)
{
	int errnum;

	void* main_thread_result;
	if ( (errnum = pthread_join(main_thread, &main_thread_result)) )
		test_error(errnum, "pthread_join");

	test_assert(expected_result == &main_thread);

	exit(0);
}

int main(void)
{
	int errnum;

	main_thread = pthread_self();

	void* expected_result = &main_thread;

	pthread_t thread;
	if ( (errnum = pthread_create(&thread, NULL, &thread_routine, expected_result)) )
		test_error(errnum, "pthread_create");

	pthread_exit(expected_result);
}
