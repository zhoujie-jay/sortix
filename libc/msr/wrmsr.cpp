/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013

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

    msr/wrmsr.cpp
    Sets a model specific register.

*******************************************************************************/

#include <sys/syscall.h>

#include <stdint.h>
#include <msr.h>

#if defined(__i386__) || defined(__x86_64__)

DEFN_SYSCALL2(uint64_t, sys_wrmsr, SYSCALL_WRMSR, uint32_t, uint64_t);

extern "C" uint64_t wrmsr(uint32_t msrid, uint64_t value)
{
	return sys_wrmsr(msrid, value);
}

#endif
