/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2014.

    This file is part of Sortix libpthread.

    Sortix libpthread is free software: you can redistribute it and/or modify it
    under the terms of the GNU Lesser General Public License as published by the
    Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    Sortix libpthread is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
    License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with Sortix libpthread. If not, see <http://www.gnu.org/licenses/>.

    pthread_initialize.c++
    Prepares the process for pthread usage.

*******************************************************************************/

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sortix/elf-note.h>

// Emit an ELF note containing the size and alignment of struct pthread.
__attribute__((used))
static void elf_note_sortix_pthread_size()
{
	asm volatile (
		".pushsection .note.sortix,\"a\",@note\n\t"
		".align 4\n\t"
		".long 2f-1f\n\t" // namesz
		".long 4f-3f\n\t" // descsz
		".long %c0\n" // type
		"1:\n\t"
		".string \"Sortix\"\n" // name
		"2:\n\t"
		".align 4\n"
		"3:\n\t"
#if defined(__x86_64__)
		".quad %c1\n"
		".quad %c2\n"
#elif defined(__i386__)
		".long %c1\n"
		".long %c2\n"
#endif
		"4:\n\t"
		".align 4\n\t"
		".popsection\n\t"
		:: "n"(ELF_NOTE_SORTIX_UTHREAD_SIZE),
		   "n"(sizeof(struct pthread)),
		   "n"(alignof(struct pthread))
	);
}

extern "C" void pthread_initialize(void)
{
}
