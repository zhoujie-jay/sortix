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

    sys/un.h
    Unix domain socket definitions.

*******************************************************************************/

#ifndef _SYS_UN_H
#define _SYS_UN_H 1

#include <sys/cdefs.h>

#include <sys/__/types.h>

__BEGIN_DECLS

@include(sa_family_t.h)

struct sockaddr_un
{
	sa_family_t sun_family;
	char sun_path[128 - sizeof(sa_family_t)];
};

__END_DECLS

#endif
