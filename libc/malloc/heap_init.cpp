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

    malloc/heap_init.cpp
    Initializes the dynamic heap.

*******************************************************************************/

#include <malloc.h>
#if __STDC_HOSTED__
#include <pthread.h>
#endif
#include <string.h>

#if __is_sortix_kernel
#include <sortix/kernel/kthread.h>
#endif

extern "C" { struct heap_state __heap_state; }

#if __STDC_HOSTED__
extern "C" { pthread_mutex_t __heap_mutex = PTHREAD_MUTEX_INITIALIZER; }
#elif __is_sortix_kernel
extern "C" { Sortix::kthread_mutex_t __heap_mutex = Sortix::KTHREAD_MUTEX_INITIALIZER; }
#endif
