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

    signal/sigandset.c
    Computes the intersection of two signal sets.

*******************************************************************************/

#include <stdint.h>
#include <signal.h>

int sigandset(sigset_t* set, const sigset_t* left, const sigset_t* right)
{
	for ( size_t i = 0; i < sizeof(set->__val) / sizeof(set->__val[0]); i++ )
		set->__val[i] = left->__val[i] & right->__val[i];
	return 0;
}
