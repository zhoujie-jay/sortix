/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2016.

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

    sortix/ioctl.h
    Miscellaneous device control interface.

*******************************************************************************/

#ifndef INCLUDE_SORTIX_IOCTL_H
#define INCLUDE_SORTIX_IOCTL_H

#define __IOCTL_TYPE_EXP 3 /* 2^3 kinds of argument types supported.*/
#define __IOCTL_TYPE_MASK ((1 << __IOCTL_TYPE_EXP) - 1)
#define __IOCTL_TYPE_VOID 0
#define __IOCTL_TYPE_INT 1
#define __IOCTL_TYPE_LONG 2
#define __IOCTL_TYPE_PTR 3
/* 4-7 is unused in case of future expansion. */
#define __IOCTL(index, type) ((index) << __IOCTL_TYPE_EXP | (type))
#define __IOCTL_INDEX(value) ((value) >> __IOCTL_TYPE_EXP)
#define __IOCTL_TYPE(value) ((value) & __IOCTL_TYPE_MASK)

#define TIOCGWINSZ __IOCTL(1, __IOCTL_TYPE_PTR)

#endif
