/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012, 2013, 2014.

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

    sortix/__/stat.h
    Defines the struct stat used for file meta-information and other useful
    macros and values relating to values stored in it.

*******************************************************************************/

#ifndef INCLUDE_SORTIX____STAT_H
#define INCLUDE_SORTIX____STAT_H

#include <sys/cdefs.h>

#include <sortix/__/dt.h>

__BEGIN_DECLS

#define __S_IFMT_SHIFT 12
#define __S_IFMT_MASK __DT_BITS

__END_DECLS

#endif
