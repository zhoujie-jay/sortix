/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2014.

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

    semaphore.h
    Semaphore API.

*******************************************************************************/

#ifndef INCLUDE_SEMAPHORE_H
#define INCLUDE_SEMAPHORE_H

#include <sys/cdefs.h>

#include <sortix/timespec.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
#if defined(__is_sortix_libpthread)
	int value;
#else
	int __value;
#endif
} sem_t;

#define SEM_FAILED ((sem_t*) 0)

/*int sem_close(sem_t*);*/
int sem_destroy(sem_t*);
int sem_getvalue(sem_t* __restrict, int* __restrict);
int sem_init(sem_t*, int, unsigned int);
/*sem_t* sem_open(const char*, int, ...);*/
int sem_post(sem_t*);
int sem_timedwait(sem_t* __restrict, const struct timespec* __restrict);
int sem_trywait(sem_t*);
/*int sem_unlink(const char*);*/
int sem_wait(sem_t*);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
