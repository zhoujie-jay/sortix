/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2014.

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

    random.cpp
    Kernel entropy gathering.

*******************************************************************************/

#include <errno.h>

#include <sortix/kernel/copy.h>
#include <sortix/kernel/random.h>
#include <sortix/kernel/syscall.h>

namespace Sortix {
namespace Random {

int sys_getentropy(void* user_buffer, size_t size)
{
	unsigned char buffer[256];
	if ( sizeof(buffer) < size )
		return errno = EIO, -1;
	// TODO: SECURITY: We need to actually gather entropy and deliver it.
	for ( size_t i = 0; i < size; i++ )
		buffer[i] = i;
	if ( !CopyToUser(user_buffer, buffer, size) )
		return -1;
	return 0;
}

void Init()
{
	Syscall::Register(SYSCALL_GETENTROPY, (void*) sys_getentropy);
}

} // namespace Random
} // namespace Sortix
