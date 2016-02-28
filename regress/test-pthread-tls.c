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

    test-pthread-tls.c
    Tests whether basic tls support works.

*******************************************************************************/

#include <pthread.h>

#include "test.h"

__thread int tls_variable = 42;

void* thread_routine(void* ctx)
{
	(void) ctx;

	test_assert(tls_variable == 42);

	tls_variable = 9001;

	return NULL;
}

int main(void)
{
	int errnum;

	test_assert(tls_variable == 42);

	tls_variable = 1337;

	pthread_t thread;
	if ( (errnum = pthread_create(&thread, NULL, &thread_routine, NULL)) )
		test_error(errnum, "pthread_create");

	if ( (errnum = pthread_join(thread, NULL)) )
		test_error(errnum, "pthread_join");

	test_assert(tls_variable == 1337);

	return 0;
}
