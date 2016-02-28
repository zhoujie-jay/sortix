/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2014, 2015.

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

    ssp/__stack_chk_fail.c
    Abnormally terminate the process on stack smashing.

*******************************************************************************/

#include <errno.h>
#include <fcntl.h>
#include <scram.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <__/wordsize.h>

#ifdef __is_sortix_libk
#include <libk.h>
#endif

#if __WORDSIZE == 32
#define STACK_CHK_GUARD 0x01234567
#elif __WORDSIZE == 64
#define STACK_CHK_GUARD 0x0123456789ABCDEF
#endif

#ifdef __is_sortix_libk
/* TODO: Have this filled in by the program loader. */
#else
/* TODO: Have this filled in by the boot loader. */
#endif
uintptr_t __stack_chk_guard = STACK_CHK_GUARD;

__attribute__((noreturn))
void __stack_chk_fail(void)
{
#ifdef __is_sortix_libk
	libk_stack_chk_fail();
#else
	scram(SCRAM_STACK_SMASH, NULL);
#endif
}
