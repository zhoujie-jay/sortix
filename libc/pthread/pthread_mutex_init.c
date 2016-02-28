/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2014.

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

    pthread/pthread_mutex_init.c
    Initializes a mutex.

*******************************************************************************/

#include <pthread.h>

int pthread_mutex_init(pthread_mutex_t* restrict mutex,
                       const pthread_mutexattr_t* restrict attr)
{
	pthread_mutexattr_t default_attr;
	if ( !attr )
	{
		pthread_mutexattr_init(&default_attr);
		attr = &default_attr;
	}

	mutex->lock = 0;
	mutex->type = attr->type;
	mutex->owner = 0;
	mutex->recursion = 0;

	return 0;
}
