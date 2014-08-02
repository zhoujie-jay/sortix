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

    sem_getvalue.c++
    Get the value of a semaphore.

*******************************************************************************/

#include <semaphore.h>

extern "C" int sem_getvalue(sem_t* restrict sem, int* restrict value_ptr)
{
	*value_ptr = __atomic_load_n(&sem->value, __ATOMIC_SEQ_CST);
	return 0;
}
