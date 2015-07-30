/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2015.

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

    psctl.h
    Process control interface.

*******************************************************************************/

#ifndef INCLUDE_SYS_IOCTL_H
#define INCLUDE_SYS_IOCTL_H

#include <sys/cdefs.h>

#include <sys/__/types.h>

#include <sortix/psctl.h>

#ifndef __pid_t_defined
#define __pid_t_defined
typedef __pid_t pid_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

int psctl(pid_t, int, ...);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
