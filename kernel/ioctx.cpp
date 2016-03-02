/*
 * Copyright (c) 2012, 2013, 2014 Jonas 'Sortie' Termansen.
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
 * ioctx.cpp
 * The context for io operations: who made it, how should data be copied, etc.
 */

#include <sortix/kernel/copy.h>
#include <sortix/kernel/ioctx.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/process.h>

namespace Sortix {

void SetupUserIOCtx(ioctx_t* ctx)
{
	Process* process = CurrentProcess();
	ScopedLock lock(&process->idlock);
	ctx->uid = ctx->auth_uid = process->uid;
	ctx->gid = ctx->auth_gid = process->gid;
	ctx->copy_to_dest = CopyToUser;
	ctx->copy_from_src = CopyFromUser;
	ctx->zero_dest = ZeroUser;
	ctx->dflags = 0;
}

void SetupKernelIOCtx(ioctx_t* ctx)
{
	Process* process = CurrentProcess();
	ScopedLock lock(&process->idlock);
	ctx->uid = ctx->auth_uid = process->uid;
	ctx->gid = ctx->auth_gid = process->gid;
	ctx->copy_to_dest = CopyToKernel;
	ctx->copy_from_src = CopyFromKernel;
	ctx->zero_dest = ZeroKernel;
	ctx->dflags = 0;
}

} // namespace Sortix
