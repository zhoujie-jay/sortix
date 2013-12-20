/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012, 2013.

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

    sortix/termios.h
    Defines values for termios.

*******************************************************************************/

#ifndef INCLUDE_SORTIX_TERMIOS_H
#define INCLUDE_SORTIX_TERMIOS_H

#include <sys/cdefs.h>

#include <sys/__/types.h>

__BEGIN_DECLS

#ifndef __size_t_defined
#define __size_t_defined
#define __need_size_t
#include <stddef.h>
#endif

struct wincurpos
{
	size_t wcp_row;
	size_t wcp_col;
};

struct winsize
{
	size_t ws_row;
	size_t ws_col;
	size_t ws_xpixel;
	size_t ws_ypixel;
};

__END_DECLS

#endif
