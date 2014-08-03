/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2014.

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

    dirent/dregister.cpp
    Registers an directory stream for later automatic closing.

*******************************************************************************/

#include <dirent.h>
#include <pthread.h>

extern "C" void dregister(DIR* dir)
{
	pthread_mutex_lock(&__first_dir_lock);
	dir->flags |= _DIR_REGISTERED;
	if ( (dir->next = __first_dir) )
		dir->next->prev = dir;
	__first_dir = dir;
	pthread_mutex_unlock(&__first_dir_lock);
}
