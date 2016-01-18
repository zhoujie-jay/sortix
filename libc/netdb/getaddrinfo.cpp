/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2015, 2016.

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

    netdb/getaddrinfo.cpp
    Network address and service translation.

*******************************************************************************/

#include <sys/socket.h>

#include <ctype.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>

static bool linkaddrinfo(struct addrinfo** restrict* res_ptr,
                         const struct addrinfo* restrict templ)
{
	struct addrinfo* link = (struct addrinfo*) calloc(1, sizeof(struct addrinfo));
	if ( !link )
		return false;
	link->ai_flags = templ->ai_flags;
	link->ai_family = templ->ai_family;
	link->ai_socktype = templ->ai_socktype;
	link->ai_protocol = templ->ai_protocol;
	link->ai_addrlen = templ->ai_addrlen;
	link->ai_addr = (struct sockaddr*) malloc(templ->ai_addrlen);
	if ( !link->ai_addr )
		return free(link), false;
	memcpy(link->ai_addr, templ->ai_addr, templ->ai_addrlen);
	link->ai_canonname = templ->ai_canonname ? strdup(templ->ai_canonname) : NULL;
	if ( templ->ai_canonname && !link->ai_canonname )
		return free(link), free(link->ai_addr), false;
	**res_ptr = link;
	*res_ptr = &link->ai_next;
	return true;
}

extern "C" int getaddrinfo(const char* restrict node,
                           const char* restrict servname,
                           const struct addrinfo* restrict hints,
                           struct addrinfo** restrict res)
{
	struct addrinfo hints_def;
	if ( !hints )
	{
		memset(&hints_def, 0, sizeof(hints_def));
		hints_def.ai_family = AF_UNSPEC;
		hints = &hints_def;
	}

	int socktype = hints->ai_socktype;
	if ( socktype == 0 )
		socktype = SOCK_STREAM;

	in_port_t port = 0;
	if ( servname )
	{
		if ( isspace((unsigned char) servname[0]) )
			return EAI_SERVICE;
		const char* end;
		long portl = strtol(servname, (char**) &end, 10);
		if ( end[0] )
			return EAI_SERVICE;
		if ( (in_port_t) portl != portl )
			return EAI_SERVICE;
		port = portl;
	}

	struct addrinfo** res_orig = res;
	*res = NULL;

	if ( !node )
	{
		bool any = false;
		if ( hints->ai_family == AF_UNSPEC || hints->ai_family == AF_INET )
		{
			any = true;
			struct sockaddr_in sin;
			memset(&sin, 0, sizeof(sin));
			sin.sin_family = AF_INET;
			sin.sin_port = htons(port);
			memcpy(&sin.sin_addr, "\x7F\x00\x00\x01", 4);
			struct addrinfo templ;
			memset(&templ, 0, sizeof(templ));
			templ.ai_family = sin.sin_family;
			templ.ai_socktype = socktype;
			templ.ai_protocol = hints->ai_protocol;
			templ.ai_addrlen = sizeof(sin);
			templ.ai_addr = (struct sockaddr*) &sin;
			if ( !linkaddrinfo(&res, &templ) )
				return freeaddrinfo(*res_orig), EAI_MEMORY;
		}
		if ( any )
			return 0;
	}

	return EAI_NONAME;
}
