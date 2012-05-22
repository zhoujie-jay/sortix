/*******************************************************************************

	Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

	This file is part of LibMaxsi.

	LibMaxsi is free software: you can redistribute it and/or modify it under
	the terms of the GNU Lesser General Public License as published by the Free
	Software Foundation, either version 3 of the License, or (at your option)
	any later version.

	LibMaxsi is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
	details.

	You should have received a copy of the GNU Lesser General Public License
	along with LibMaxsi. If not, see <http://www.gnu.org/licenses/>.

	getcwd.cpp
	Returns the current working directory.

*******************************************************************************/

#include <libmaxsi/platform.h>
#include <libmaxsi/syscall.h>

namespace Maxsi {

DEFN_SYSCALL2(char*, SysGetCWD, SYSCALL_GETCWD, char*, size_t);

extern "C" char* getcwd(char* buf, size_t size)
{
	return SysGetCWD(buf, size);
}

} // namespace Maxsi