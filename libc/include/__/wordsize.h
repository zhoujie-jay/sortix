/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2013.

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

    __/wordsize.h
    Native machine word size.

*******************************************************************************/

#ifndef INCLUDE____WORDSIZE_H
#define INCLUDE____WORDSIZE_H

#if defined(__x86_64__)
	#define __WORDSIZE 64
#elif defined(__i386__)
	#define __WORDSIZE 32
#else
	#error "You need to detect your CPU and declare __WORDSIZE appropriately"
#endif

#endif
