/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012, 2013, 2014.

    This file is part of Sortix.

    Sortix is free software: you can redistribute it and/or modify it under the
    terms of the GNU General Public License as published by the Free Software
    Foundation, either version 3 of the License, or (at your option) any later
    version.

    Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
    details.

    You should have received a copy of the GNU General Public License along with
    Sortix. If not, see <http://www.gnu.org/licenses/>.

    ioctx.cpp
    The context for io operations: who made it, how should data be copied, etc.

*******************************************************************************/

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
