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

    malloc/__heap_unlock.cpp
    Unlocks the dynamic heap.

*******************************************************************************/

#include <malloc.h>
#if __STDC_HOSTED__
#include <pthread.h>
#endif

#if __is_sortix_kernel
#include <sortix/kernel/kthread.h>
#endif

#if __STDC_HOSTED__
extern "C" pthread_mutex_t __heap_mutex;
#elif __is_sortix_kernel
extern "C" Sortix::kthread_mutex_t __heap_mutex;
#endif

extern "C" void __heap_unlock(void)
{
#if __STDC_HOSTED__
	pthread_mutex_unlock(&__heap_mutex);
#elif __is_sortix_kernel
	Sortix::kthread_mutex_unlock(&__heap_mutex);
#endif
}
