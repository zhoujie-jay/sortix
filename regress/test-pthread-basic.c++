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

    test-pthread-basic.c++
    Tests whether basic pthread support works.

*******************************************************************************/

#include <pthread.h>

#include "test.h"

void* thread_routine(void* cookie)
{
	int* test_failure_ptr = (int*) cookie;
	*test_failure_ptr = 0;
	return cookie;
}

int main(void)
{
	int errnum;

	int test_failure = 1;

	pthread_t thread;
	if ( (errnum = pthread_create(&thread, NULL, &thread_routine, &test_failure)) )
		test_error(errnum, "pthread_create");

	void* thread_result;
	if ( (errnum = pthread_join(thread, &thread_result)) )
		test_error(errnum, "pthread_join");

	test_assert(test_failure == 0);
	test_assert(thread_result == &test_failure);

	return 0;
}
