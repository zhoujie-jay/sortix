/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2014.

    This file is part of Sortix.

    Sortix is free software: you can redistribute it and/or modify it under the
    terms of the GNU General Public License as published by the Free Software
    Foundation, either version 3 of the License, or (at your option) any later
    version.

    Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
    details.

    You should have received a copy of the GNU General Public License along with
    Sortix. If not, see <http://www.gnu.org/licenses/>.

    sortix/sigevent.h
    Declares the sigevent structure.

*******************************************************************************/

#ifndef INCLUDE_SORTIX_SIGEVENT_H
#define INCLUDE_SORTIX_SIGEVENT_H

#include <sys/cdefs.h>

#if __STDC_HOSTED__
#include <__/pthread.h>
#endif

#include <sortix/sigval.h>

__BEGIN_DECLS

#if __STDC_HOSTED__
#ifndef __pthread_attr_t_defined
#define __pthread_attr_t_defined
typedef __pthread_attr_t pthread_attr_t;
#endif
#endif

#define SIGEV_NONE 0
#define SIGEV_SIGNAL 1
#define SIGEV_THREAD 2

struct sigevent
{
	int sigev_notify;
	int sigev_signo;
	union sigval sigev_value;
	void (*sigev_notify_function)(union sigval);
#if __STDC_HOSTED__
	pthread_attr_t* sigev_notify_attributes;
#else
	void* sigev_notify_attributes;
#endif
};

__END_DECLS

#endif
