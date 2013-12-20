/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012.

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

    sortix/kernel/ioctx.h
    The context for io operations: who made it, how should data be copied, etc.

*******************************************************************************/

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
	int dflags;
};

typedef struct ioctx_struct ioctx_t;

void SetupUserIOCtx(ioctx_t* ctx);
void SetupKernelIOCtx(ioctx_t* ctx);

} // namespace Sortix

#endif
