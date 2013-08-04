/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

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

    unistd/sysconf.cpp
    Get configuration information at runtime.

*******************************************************************************/

#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

extern "C" long sysconf(int name)
{
	switch ( name )
	{
	case _SC_CLK_TCK: return 1000;
	case _SC_PAGESIZE: case _SC_PAGE_SIZE:
		return getpagesize();
	case _SC_OPEN_MAX: return 0x10000;
	case _SC_RTSIG_MAX: return (SIGRTMAX+1) - SIGRTMIN;
	default:
		fprintf(stderr, "%s:%u warning: %s(%i) is unsupported\n",
		        __FILE__, __LINE__, __func__, name);
		return errno = EINVAL, -1;
	}
}
