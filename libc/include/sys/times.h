/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

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

    sys/times.h
    Declaration for the times function.

*******************************************************************************/

#ifndef INCLUDE_SYS_TIMES_H
#define INCLUDE_SYS_TIMES_H

#include <sys/cdefs.h>

#include <sys/__/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __clock_t_defined
#define __clock_t_defined
typedef __clock_t clock_t;
#endif

struct tms
{
	clock_t tms_utime;
	clock_t tms_stime;
	clock_t tms_cutime;
	clock_t tms_cstime;
};

clock_t times(struct tms*);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
