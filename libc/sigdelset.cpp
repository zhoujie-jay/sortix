/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

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

    sigdelset.cpp
    Remove a signal from a signal set.

*******************************************************************************/

#include <errno.h>
#include <signal.h>
#include <stdint.h>

extern "C" int sigdelset(sigset_t* set, int signum)
{
	int max_signals = sizeof(set->__val) * 8;
	if ( max_signals <= signum )
		return errno = EINVAL, -1;
	size_t which_byte = signum / 8;
	size_t which_bit  = signum % 8;
	uint8_t* bytes = (uint8_t*) set->__val;
	bytes[which_byte] &= ~(1 << which_bit);
	return 0;
}
