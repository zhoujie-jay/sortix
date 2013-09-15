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

    pthread_self.c++
    Returns the identity of the current thread.

*******************************************************************************/

#include <pthread.h>

extern "C" pthread_t pthread_self(void)
{
	pthread_t current_thread;
#if defined(__i386__)
	asm ( "mov %%gs:0, %0" : "=r"(current_thread));
#elif defined(__x86_64__)
	asm ( "mov %%fs:0, %0" : "=r"(current_thread));
#else
	#error "You need to implement pthread_self for your platform"
#endif
	return current_thread;
}
