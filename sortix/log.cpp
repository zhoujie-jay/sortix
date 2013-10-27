/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013.

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

    log.cpp
    A system for logging various messages to the kernel log.

*******************************************************************************/

#include <stddef.h>
#include <string.h>

#include <sortix/kernel/kernel.h>
#include <sortix/kernel/log.h>
#include <sortix/kernel/syscall.h>

namespace Sortix {
namespace Log {

size_t (*device_callback)(void*, const char*, size_t) = NULL;
size_t (*device_width)(void*) = NULL;
size_t (*device_height)(void*) = NULL;
bool (*device_sync)(void*) = NULL;
void* device_pointer = NULL;

void Init(size_t (*callback)(void*, const char*, size_t),
          size_t (*widthfunc)(void*),
          size_t (*heightfunc)(void*),
          bool (*syncfunc)(void*),
          void* user)
{
	device_callback = callback;
	device_width = widthfunc;
	device_height = heightfunc;
	device_sync = syncfunc;
	device_pointer = user;
}

} // namespace Log
} // namespace Sortix
