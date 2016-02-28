/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2014.

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

    unistd/getentropy.c
    Read entropy into an user buffer.

*******************************************************************************/

#include <sys/syscall.h>

#include <unistd.h>

DEFN_SYSCALL2(int, sys_getentropy, SYSCALL_GETENTROPY, void*, size_t);

int getentropy(void* buffer, size_t size)
{
	return sys_getentropy(buffer, size);
}
