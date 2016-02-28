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

    test-pthread-self.c
    Tests whether basic pthread_self() support works.

*******************************************************************************/

#include <pthread.h>

#include "test.h"

static pthread_t child_thread_self;

void* thread_routine(void* main_thread_ptr)
{
	pthread_t main_thread = *(pthread_t*) main_thread_ptr;

	test_assert(!pthread_equal(main_thread, pthread_self()));

	child_thread_self = pthread_self();

	return NULL;
}

int main(void)
{
	int errnum;

	pthread_t main_thread = pthread_self();

	pthread_t thread;
	if ( (errnum = pthread_create(&thread, NULL, &thread_routine, &main_thread)) )
		test_error(errnum, "pthread_create");

	if ( (errnum = pthread_join(thread, NULL)) )
		test_error(errnum, "pthread_join");

	test_assert(!pthread_equal(thread, pthread_self()));
	test_assert(pthread_equal(thread, child_thread_self));
	test_assert(!pthread_equal(pthread_self(), child_thread_self));

	return 0;
}
