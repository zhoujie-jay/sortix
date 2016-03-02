/*
 * Copyright (c) 2012 Jonas 'Sortie' Termansen.
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
 * sortix/mman.h
 * Memory management declarations.
 */

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

#define MAP_ANONYMOUS (1<<2)
#define MAP_FIXED (1<<3)

#define MAP_FAILED ((void*) -1)

#endif
