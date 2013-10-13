/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012.

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

    sys/display.h
    Display message passing.

*******************************************************************************/

#ifndef INCLUDE_SYS_DISPLAY_H
#define INCLUDE_SYS_DISPLAY_H

#include <sys/cdefs.h>

#include <stddef.h>
#include <stdint.h>
#include <sortix/display.h>

__BEGIN_DECLS

int dispmsg_issue(void* ptr, size_t size);

__END_DECLS

#endif
