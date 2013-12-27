/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011.

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

    sys/time.h
    Time types.

*******************************************************************************/

#ifndef _SYS_TIME_H
#define _SYS_TIME_H 1

#include <sys/cdefs.h>

#include <sys/__/types.h>

__BEGIN_DECLS

#ifndef __time_t_defined
#define __time_t_defined
typedef __time_t time_t;
#endif

#ifndef __suseconds_t_defined
#define __suseconds_t_defined
typedef __suseconds_t suseconds_t;
#endif

#ifndef __timeval_defined
#define __timeval_defined
struct timeval
{
	time_t tv_sec;
	suseconds_t tv_usec;
};
#endif

int gettimeofday(struct timeval* __restrict tp, void* __restrict tzp);

__END_DECLS

#endif
