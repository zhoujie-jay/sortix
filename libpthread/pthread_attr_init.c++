/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2014, 2015.

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

    pthread_attr_init.c++
    Initialize a thread attributes object.

*******************************************************************************/

#include <pthread.h>
#include <string.h>

static const unsigned long DEFAULT_STACK_SIZE = 80 * 1024;

extern "C" int pthread_attr_init(pthread_attr_t* attr)
{
	memset(attr, 0, sizeof(*attr));
	attr->stack_size = DEFAULT_STACK_SIZE;
	attr->detach_state = PTHREAD_CREATE_JOINABLE;
	return 0;
}
