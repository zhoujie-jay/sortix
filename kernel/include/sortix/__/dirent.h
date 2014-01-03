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

    sortix/__/dirent.h
    Format of directory entries.

*******************************************************************************/

#ifndef INCLUDE_SORTIX____DIRENT_H
#define INCLUDE_SORTIX____DIRENT_H

#include <sys/cdefs.h>

#include <sortix/__/dt.h>
#include <sortix/__/stat.h>

__BEGIN_DECLS

#define __IFTODT(mode) (((mode) & __S_IFMT) >> __S_IFMT_SHIFT)
#define __DTTOIF(dirtype) ((dirtype) << __S_IFMT_SHIFT)

__END_DECLS

#endif
