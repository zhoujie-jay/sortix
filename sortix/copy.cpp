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

    copy.h
    The context for io operations: who made it, how should data be copied, etc.

*******************************************************************************/

#include <string.h>

#include <sortix/kernel/copy.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/string.h>

namespace Sortix {

// TODO: These are currently insecure, please check userspace tables before
// moving data to avoid security problems.

bool CopyToUser(void* userdst, const void* ksrc, size_t count)
{
	memcpy(userdst, ksrc, count);
	return true;
}

bool CopyFromUser(void* kdst, const void* usersrc, size_t count)
{
	//Log::PrintF("[copy.cpp] Copying %zu bytes from 0x%zx to 0x%zx\n", count, usersrc, kdst);
	memcpy(kdst, usersrc, count);
	return true;
}

bool CopyToKernel(void* kdst, const void* ksrc, size_t count)
{
	memcpy(kdst, ksrc, count);
	return true;
}

bool CopyFromKernel(void* kdst, const void* ksrc, size_t count)
{
	memcpy(kdst, ksrc, count);
	return true;
}

char* GetStringFromUser(const char* str)
{
	return String::Clone(str);
}

} // namespace Sortix
