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

    pthread/pthread_cond_init.c
    Initializes a condition variable.

*******************************************************************************/

#include <pthread.h>

int pthread_cond_init(pthread_cond_t* restrict cond,
                       const pthread_condattr_t* restrict attr)
{
	pthread_condattr_t default_attr;
	if ( !attr )
	{
		pthread_condattr_init(&default_attr);
		attr = &default_attr;
	}

	cond->first = NULL;
	cond->last = NULL;
	cond->clock = attr->clock;

	return 0;
}
