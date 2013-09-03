/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

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

    pthread_cond_signal.c++
    Signals a condition.

*******************************************************************************/

#include <pthread.h>

extern "C" int pthread_cond_signal(pthread_cond_t* cond)
{
	struct pthread_cond_elem* elem = cond->first;
	if ( !elem )
		return 0;
	if ( !(cond->first = elem->next) )
		cond->last = NULL;
	elem->next = NULL;
	elem->woken = 1;
	return 0;
}
