/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013

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

    utime.h
    Deprecated interface to change the file access and moficiation times.

*******************************************************************************/

#ifndef INCLUDE_UTIME_H
#define INCLUDE_UTIME_H

#include <sys/cdefs.h>

#include <sys/__/types.h>

__BEGIN_DECLS

#ifndef __time_t_defined
#define __time_t_defined
typedef __time_t time_t;
#endif

struct utimbuf
{
	time_t actime;
	time_t modtime;
};

int utime(const char* filename, const struct utimbuf* times);

__END_DECLS

#endif
