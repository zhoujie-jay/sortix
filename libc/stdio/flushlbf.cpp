/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013.

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

    stdio/flushlbf.cpp
    Flushes all line buffered registered files.

*******************************************************************************/

#include <pthread.h>
#include <stdio.h>

extern "C" void flushlbf(void)
{
	pthread_mutex_lock(&__first_file_lock);
	for ( FILE* fp = __first_file; fp; fp = fp->next )
		fflush(fp);
	pthread_mutex_unlock(&__first_file_lock);
}
