/*
 * Copyright (c) 2012, 2014 Jonas 'Sortie' Termansen.
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
 * sortix/kernel/ioctx.h
 * The context for io operations: who made it, how should data be copied, etc.
 */

#ifndef SORTIX_IOCTX_H
#define SORTIX_IOCTX_H

#include <sys/types.h>

#include <stdint.h>

namespace Sortix {

struct ioctx_struct
{
	uid_t uid, auth_uid;
	gid_t gid, auth_gid;
	bool (*copy_to_dest)(void* dest, const void* src, size_t n);
	bool (*copy_from_src)(void* dest, const void* src, size_t n);
	bool (*zero_dest)(void* dest, size_t n);
	int dflags;
};

typedef struct ioctx_struct ioctx_t;

void SetupUserIOCtx(ioctx_t* ctx);
void SetupKernelIOCtx(ioctx_t* ctx);

} // namespace Sortix

#endif
