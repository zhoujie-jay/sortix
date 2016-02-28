/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2014.

    This file is part of the Sortix C Library.

    The Sortix C Library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    The Sortix C Library is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
    License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with the Sortix C Library. If not, see <http://www.gnu.org/licenses/>.

    semaphore/sem_post.c
    Unlock a semaphore.

*******************************************************************************/

#include <errno.h>
#include <limits.h>
#include <semaphore.h>
#include <stdbool.h>

int sem_post(sem_t* sem)
{
	while ( true )
	{
		int old_value = __atomic_load_n(&sem->value, __ATOMIC_SEQ_CST);
		if ( old_value == INT_MAX )
			return errno = EOVERFLOW;

		int new_value = old_value + 1;
		if ( !__atomic_compare_exchange_n(&sem->value, &old_value, new_value,
		                                  false,
		                                  __ATOMIC_SEQ_CST, __ATOMIC_RELAXED) )
			continue;

		return 0;
	}
}
