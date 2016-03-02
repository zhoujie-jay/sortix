/*
 * Copyright (c) 2011, 2012, 2013, 2014 Jonas 'Sortie' Termansen.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * init/init.c
 * Initializes the standard library.
 */

#include <elf.h>
#include <malloc.h>
#include <pthread.h>
#include <string.h>

char* program_invocation_name;
char* program_invocation_short_name;

static char* find_last_elem(char* str)
{
	size_t len = strlen(str);
	for ( size_t i = len; i; i-- )
		if ( str[i-1] == '/' )
			return str + i;
	return str;
}

// Emit an ELF note containing the size and alignment of struct pthread.
__attribute__((used))
static void elf_note_sortix_pthread_size(void)
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

void initialize_standard_library(int argc, char* argv[])
{
	const char* argv0 = argc ? argv[0] : "";
	program_invocation_name = (char*) argv0;
	program_invocation_short_name = find_last_elem((char*) argv0);

	struct pthread* self = pthread_self();
	self->join_lock = (pthread_mutex_t) PTHREAD_NORMAL_MUTEX_INITIALIZER_NP;
	self->join_lock.lock = 1 /* LOCKED_VALUE */;
	self->join_lock.type = PTHREAD_MUTEX_NORMAL;
	self->join_lock.owner = (unsigned long) self;
	self->detach_lock = (pthread_mutex_t) PTHREAD_NORMAL_MUTEX_INITIALIZER_NP;
	self->detach_state = PTHREAD_CREATE_JOINABLE;
}
