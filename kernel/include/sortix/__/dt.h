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

    sortix/__/dt.h
    Defines the values for the d_type field.

*******************************************************************************/

#ifndef INCLUDE_SORTIX____DT_H
#define INCLUDE_SORTIX____DT_H

#include <sys/cdefs.h>

__BEGIN_DECLS

#define __DT_UNKNOWN 0x0
#define __DT_FIFO 0x1
#define __DT_CHR 0x2
#define __DT_DIR 0x4
#define __DT_BLK 0x6
#define __DT_REG 0x8
#define __DT_LNK 0xA
#define __DT_SOCK 0xC
#define __DT_BITS 0xF

__END_DECLS

#endif
