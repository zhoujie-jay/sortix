/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

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

__BEGIN_DECLS

#define SIGEV_NONE 0
#define SIGEV_SIGNAL 1
#define SIGEV_THREAD 2

union sigval
{
	int sival_int;
	void* sival_ptr;
};

struct sigevent
{
	int sigev_notify;
	int sigev_signo;
	union sigval sigev_value;
	void (*sigev_notify_function)(union sigval);
	/*pthread_attr_t* sigev_notify_attributes;*/
	void* sigev_notify_attributes;
};

__END_DECLS

#endif
