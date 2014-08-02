/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2014.

    This file is part of Sortix libpthread.

    Sortix libpthread is free software: you can redistribute it and/or modify it
    under the terms of the GNU Lesser General Public License as published by the
    Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    Sortix libpthread is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
    License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with Sortix libpthread. If not, see <http://www.gnu.org/licenses/>.

    sem_trywait.c++
    Lock a semaphore.

*******************************************************************************/

#include <errno.h>
#include <semaphore.h>

extern "C" int sem_trywait(sem_t* sem)
{
	int old_value = __atomic_load_n(&sem->value, __ATOMIC_SEQ_CST);
	if ( old_value <= 0 )
			return errno = EAGAIN, -1;
	int new_value = old_value - 1;
	if ( !__atomic_compare_exchange_n(&sem->value, &old_value, new_value, false,
	                                  __ATOMIC_SEQ_CST, __ATOMIC_RELAXED) )
			return errno = EAGAIN, -1;
	return 0;
}
