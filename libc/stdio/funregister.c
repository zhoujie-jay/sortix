/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2014.

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

    stdio/funregister.c
    Unregisters a FILE in the global list of open FILEs.

*******************************************************************************/

#include <pthread.h>
#include <stdio.h>

void funregister(FILE* fp)
{
	if ( !(fp->flags & _FILE_REGISTERED) )
		return;
	pthread_mutex_lock(&__first_file_lock);
	if ( !fp->prev )
		__first_file = fp->next;
	if ( fp->prev )
		fp->prev->next = fp->next;
	if ( fp->next )
		fp->next->prev = fp->prev;
	fp->flags &= ~_FILE_REGISTERED;
	pthread_mutex_unlock(&__first_file_lock);
}
