/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012.

    This file is part of the Sortix C Library.

    The Sortix C Library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    The Sortix C Library is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
    License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with the Sortix C Library. If not, see <http://www.gnu.org/licenses/>.

    sys/display/dispmsg_issue.cpp
    Send a message to the display engine.

*******************************************************************************/

#include <sys/syscall.h>
#include <sys/display.h>

DEFN_SYSCALL2(int, sys_dispmsg_issue, SYSCALL_DISPMSG_ISSUE, void*, size_t);

extern "C" int dispmsg_issue(void* ptr, size_t size)
{
	return sys_dispmsg_issue(ptr, size);
}
