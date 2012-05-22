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

	stat.cpp
	Retrieves status of a file.

*******************************************************************************/

#include <libmaxsi/platform.h>
#include <libmaxsi/syscall.h>
#include <sys/stat.h>

namespace Maxsi {

DEFN_SYSCALL2(int, SysStat, SYSCALL_STAT, const char*, struct stat*);

extern "C" int stat(const char* path, struct stat* st)
{
	return SysStat(path, st);
}

// TODO: Hack!
extern "C" int lstat(const char* path, struct stat* st)
{
	return SysStat(path, st);
}

} // namespace Maxsi