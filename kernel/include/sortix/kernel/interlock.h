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

    sortix/kernel/interlock.h
    Functions that perform non-atomic operations in an atomic manner.

*******************************************************************************/

#ifndef SORTIX_INTERLOCK_H
#define SORTIX_INTERLOCK_H

namespace Sortix {

typedef unsigned long (*ilockfunc)(unsigned long, unsigned long);
typedef struct
{
	unsigned long o; // old value
	unsigned long n; // new value
} ilret_t;

// These atomically modifies a value and return the previous value.
ilret_t InterlockedModify(unsigned long* ptr,
                          ilockfunc f,
                          unsigned long user = 0);
ilret_t InterlockedIncrement(unsigned long* ptr);
ilret_t InterlockedDecrement(unsigned long* ptr);
ilret_t InterlockedAdd(unsigned long* ptr, unsigned long arg);
ilret_t InterlockedSub(unsigned long* ptr, unsigned long arg);

} // namespace Sortix

#endif
