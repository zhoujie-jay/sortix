/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2015.

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

    netdb/gai_strerror.cpp
    Error information for getaddrinfo.

*******************************************************************************/

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>

extern "C" const char* gai_strerror(int err)
{
	switch ( err )
	{
	case EAI_AGAIN: return "Try again";
	case EAI_BADFLAGS: return "Invalid flags";
	case EAI_FAIL: return "Non-recoverable error";
	case EAI_FAMILY: return "Unrecognized address family or invalid length";
	case EAI_MEMORY: return "Out of memory";
	case EAI_NONAME: return "Name does not resolve";
	case EAI_SERVICE: return "Unrecognized service";
	case EAI_SOCKTYPE: return "Unrecognized socket type";
	case EAI_SYSTEM: return "System error";
	case EAI_OVERFLOW: return "Overflow";
	}
	return "Unknown error";
}
