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

    netinet/in.h
    Internet socket interface.

*******************************************************************************/

#ifndef INCLUDE_NETINET_IN_H
#define INCLUDE_NETINET_IN_H

#include <sys/cdefs.h>

#include <sys/__/types.h>
#include <__/endian.h>

#include <inttypes.h>

__BEGIN_DECLS

@include(in_port_t.h)
@include(in_addr_t.h)
@include(sa_family_t.h)
@include(socklen_t.h)

struct in_addr
{
	in_addr_t s_addr;
};

struct sockaddr_in
{
	sa_family_t sin_family;
	in_port_t sin_port;
	struct in_addr sin_addr;
};

struct in6_addr
{
	uint8_t s6_addr[16];
};

struct sockaddr_in6
{
	sa_family_t sin6_family;
	in_port_t sin6_port;
	uint32_t sin6_flowinfo;
	struct in6_addr sin6_addr;
	uint32_t sin6_scope_id;
};

extern const struct in6_addr in6addr_any;        /* :: */
extern const struct in6_addr in6addr_loopback;   /* ::1 */
#define IN6ADDR_ANY_INIT { { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } }
#define IN6ADDR_LOOPBACK_INIT { { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 } }

struct ipv6_mreq
{
	struct in6_addr ipv6mr_multiaddr;
	unsigned int ipv6mr_interface;
};

#define IPPROTO_IP 0
#define IPPROTO_IPV6 1
#define IPPROTO_ICMP 2
#define IPPROTO_RAW 3
#define IPPROTO_TCP 4
#define IPPROTO_UDP 5

#define INADDR_ANY ((in_addr_t) 0x00000000)
#define INADDR_BROADCAST ((in_addr_t) 0xffffffff)
#define INADDR_NONE ((in_addr_t) 0xffffffff)

#define INET_ADDRSTRLEN 16
#define INET6_ADDRSTRLEN 46

#define htons(x) __htobe16(x)
#define ntohs(x) __be16toh(x)
#define htonl(x) __htobe32(x)
#define ntohl(x) __be32toh(x)

#define IPV6_JOIN_GROUP 0
#define IPV6_LEAVE_GROUP 1
#define IPV6_MULTICAST_HOPS 2
#define IPV6_MULTICAST_IF 3
#define IPV6_MULTICAST_LOOP 4
#define IPV6_UNICAST_HOPS 5
#define IPV6_V6ONLY 6

/* TODO:
IN6_IS_ADDR_UNSPECIFIED
IN6_IS_ADDR_LOOPBACK
IN6_IS_ADDR_MULTICAST
IN6_IS_ADDR_LINKLOCAL
IN6_IS_ADDR_SITELOCAL
IN6_IS_ADDR_V4MAPPED
IN6_IS_ADDR_V4COMPAT
IN6_IS_ADDR_MC_NODELOCAL
IN6_IS_ADDR_MC_LINKLOCAL
IN6_IS_ADDR_MC_SITELOCAL
IN6_IS_ADDR_MC_ORGLOCAL
IN6_IS_ADDR_MC_GLOBAL
*/

# define IN6_ARE_ADDR_EQUAL(a,b) \
	((((__const uint32_t *) (a))[0] == ((__const uint32_t *) (b))[0])     \
	 && (((__const uint32_t *) (a))[1] == ((__const uint32_t *) (b))[1])  \
	 && (((__const uint32_t *) (a))[2] == ((__const uint32_t *) (b))[2])  \
	 && (((__const uint32_t *) (a))[3] == ((__const uint32_t *) (b))[3]))

__END_DECLS

#endif
