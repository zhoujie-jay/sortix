/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012, 2013, 2014, 2015.

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

    termios.h
    Defines values for termios.

*******************************************************************************/

/* TODO: POSIX-1.2008 compliance is only partial */

#ifndef INCLUDE_TERMIOS_H
#define INCLUDE_TERMIOS_H

#include <sys/cdefs.h>

#include <sys/__/types.h>

#if __USE_SORTIX
#include <sortix/termios.h>
#endif

__BEGIN_DECLS

#if __USE_SORTIX
#ifndef __size_t_defined
#define __size_t_defined
#define __need_size_t
#include <stddef.h>
#endif

#ifndef __ssize_t_defined
#define __ssize_t_defined
typedef __ssize_t ssize_t;
#endif
#endif

/* Functions that are Sortix extensions. */
#if __USE_SORTIX
ssize_t tcgetblob(int, const char*, void*, size_t);
ssize_t tcsetblob(int, const char*, const void*, size_t);
int tcgetwincurpos(int, struct wincurpos*);
int tcgetwinsize(int, struct winsize*);
#endif

__END_DECLS

#endif
