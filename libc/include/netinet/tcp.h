/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2014.

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

    netinet/tcp.h
    Definitions for the Internet Transmission Control Protocol.

*******************************************************************************/

#ifndef INCLUDE_NETINET_TCP_H
#define INCLUDE_NETINET_TCP_H

#include <sys/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Options at the IPPROTO_TCP socket level. */
#define TCP_NODELAY 1

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
