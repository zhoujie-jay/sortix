/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012.

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

    sortix/mman.h
    Memory management declarations.

*******************************************************************************/

#ifndef SORTIX_MMAN_H
#define SORTIX_MMAN_H

/* Note that not all combinations of the following may be possible on all
   architectures. However, you do get at least as much access as you request. */

#define PROT_NONE (0)

/* Flags that control user-space access to memory. */
#define PROT_EXEC (1<<0)
#define PROT_WRITE (1<<1)
#define PROT_READ (1<<2)
#define PROT_USER (PROT_EXEC | PROT_WRITE | PROT_READ)

/* Flags that control kernel access to memory. */
#define PROT_KEXEC (1<<3)
#define PROT_KWRITE (1<<4)
#define PROT_KREAD (1<<5)
#define PROT_KERNEL (PROT_KEXEC | PROT_KWRITE | PROT_KREAD)

#define PROT_FORK (1<<6)

#define MAP_SHARED (1<<0)
#define MAP_PRIVATE (1<<1)

#endif
