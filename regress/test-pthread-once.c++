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

    test-pthread-once.c++
    Tests whether basic pthread_once() support works.

*******************************************************************************/

#include <pthread.h>

#include "test.h"

static int init_counter = 0;
static pthread_once_t init_counter_once = PTHREAD_ONCE_INIT;

void init_counter_increase()
{
	init_counter++;
}

void* thread_routine(void*)
{
	pthread_once(&init_counter_once, init_counter_increase);

	return NULL;
}

int main(void)
{
	int errnum;

	pthread_once(&init_counter_once, init_counter_increase);

	pthread_t thread;
	if ( (errnum = pthread_create(&thread, NULL, &thread_routine, NULL)) )
		test_error(errnum, "pthread_create");

	if ( (errnum = pthread_join(thread, NULL)) )
		test_error(errnum, "pthread_join");

	test_assert(init_counter == 1);

	return 0;
}
