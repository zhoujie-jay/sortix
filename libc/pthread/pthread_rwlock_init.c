/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

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

    pthread/pthread_rwlock_init.c
    Initializes a read-write lock.

*******************************************************************************/

#include <pthread.h>

int pthread_rwlock_init(pthread_rwlock_t* restrict rwlock,
                        const pthread_rwlockattr_t* restrict attr)
{
	(void) attr;
	*rwlock = (pthread_rwlock_t) PTHREAD_RWLOCK_INITIALIZER;
	return 0;
}
