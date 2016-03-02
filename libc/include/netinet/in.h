/*
 * Copyright (c) 2013, 2014, 2015 Jonas 'Sortie' Termansen.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * netinet/in.h
 * Internet socket interface.
 */

#ifndef INCLUDE_NETINET_IN_H
#define INCLUDE_NETINET_IN_H

#include <sys/cdefs.h>

#include <sys/__/types.h>
#include <__/endian.h>

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __in_port_t_defined
#define __in_port_t_defined
typedef uint16_t in_port_t;
#endif

#ifndef __in_addr_t_defined
#define __in_addr_t_defined
typedef uint32_t in_addr_t;
#endif

#ifndef __sa_family_t_defined
#define __sa_family_t_defined
typedef unsigned short int sa_family_t;
#endif

#ifndef __socklen_t_defined
#define __socklen_t_defined
typedef __socklen_t socklen_t;
#endif

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

struct __attribute__((aligned(4))) in6_addr
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

/* #define SOL_SOCKET 0 - in <sys/socket.h> */
#define IPPROTO_ICMP 1
#define IPPROTO_IP 2
#define IPPROTO_IPV6 3
#define IPPROTO_RAW 4
#define IPPROTO_TCP 5
#define IPPROTO_UDP 6

#define INADDR_ANY        ((in_addr_t) 0x00000000)
#define INADDR_BROADCAST  ((in_addr_t) 0xffffffff)
#define INADDR_NONE       ((in_addr_t) 0xffffffff)
#define INADDR_LOOPBACK   ((in_addr_t) 0x7f000001)

#define INADDR_UNSPEC_GROUP     ((in_addr_t) 0xe0000000)
#define INADDR_ALLHOSTS_GROUP   ((in_addr_t) 0xe0000001)
#define INADDR_ALLRTRS_GROUP    ((in_addr_t) 0xe0000002)
#define INADDR_MAX_LOCAL_GROUP  ((in_addr_t) 0xe00000ff)

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

#define IN6_IS_ADDR_UNSPECIFIED(a) \
        (((uint32_t*) (a))[0] == 0 && ((uint32_t*) (a))[1] == 0 && \
         ((uint32_t*) (a))[2] == 0 && ((uint32_t*) (a))[3] == 0)
#define IN6_IS_ADDR_LOOPBACK(a) \
        (((uint32_t*) (a))[0] == 0 && ((uint32_t*) (a))[1] == 0 && \
         ((uint32_t*) (a))[2] == 0 && \
         ((uint8_t*) (a))[12] == 0 && ((uint8_t*) (a))[13] == 0 && \
         ((uint8_t*) (a))[14] == 0 && ((uint8_t*) (a))[15] == 1 )
#define IN6_IS_ADDR_MULTICAST(a) (((uint8_t*) (a))[0] == 0xff)
#define IN6_IS_ADDR_LINKLOCAL(a) \
        ((((uint8_t*) (a))[0]) == 0xfe && (((uint8_t*) (a))[1] & 0xc0) == 0x80)
#define IN6_IS_ADDR_SITELOCAL(a) \
        ((((uint8_t*) (a))[0]) == 0xfe && (((uint8_t*) (a))[1] & 0xc0) == 0xc0)
#define IN6_IS_ADDR_V4MAPPED(a) \
        (((uint32_t*) (a))[0] == 0 && ((uint32_t*) (a))[1] == 0 && \
         ((uint8_t*) (a))[8] == 0 && ((uint8_t*) (a))[9] == 0 && \
         ((uint8_t*) (a))[10] == 0xff && ((uint8_t*) (a))[11] == 0xff)
#define IN6_IS_ADDR_V4COMPAT(a) \
        (((uint32_t*) (a))[0] == 0 && ((uint32_t*) (a))[1] == 0 && \
         ((uint32_t*) (a))[2] == 0 && ((uint8_t*) (a))[15] > 1)
#define IN6_IS_ADDR_MC_NODELOCAL(a) \
        (IN6_IS_ADDR_MULTICAST(a) && ((((uint8_t*) (a))[1] & 0xf) == 0x1))
#define IN6_IS_ADDR_MC_LINKLOCAL(a) \
        (IN6_IS_ADDR_MULTICAST(a) && ((((uint8_t*) (a))[1] & 0xf) == 0x2))
#define IN6_IS_ADDR_MC_SITELOCAL(a) \
        (IN6_IS_ADDR_MULTICAST(a) && ((((uint8_t*) (a))[1] & 0xf) == 0x5))
#define IN6_IS_ADDR_MC_ORGLOCAL(a) \
        (IN6_IS_ADDR_MULTICAST(a) && ((((uint8_t*) (a))[1] & 0xf) == 0x8))
#define IN6_IS_ADDR_MC_GLOBAL(a) \
        (IN6_IS_ADDR_MULTICAST(a) && ((((uint8_t*) (a))[1] & 0xf) == 0xe))
#define __ARE_4_EQUAL(a,b) \
        (!( (0[a]-0[b]) | (1[a]-1[b]) | (2[a]-2[b]) | (3[a]-3[b]) ))
#define IN6_ARE_ADDR_EQUAL(a,b) \
        __ARE_4_EQUAL((const uint32_t*)(a), (const uint32_t*)(b))

#define IN_CLASSA(a)            ((((in_addr_t)(a)) & 0x80000000) == 0)
#define IN_CLASSA_NET           0xff000000
#define IN_CLASSA_NSHIFT        24
#define IN_CLASSA_HOST          (0xffffffff & ~IN_CLASSA_NET)
#define IN_CLASSA_MAX           128
#define IN_CLASSB(a)            ((((in_addr_t)(a)) & 0xc0000000) == 0x80000000)
#define IN_CLASSB_NET           0xffff0000
#define IN_CLASSB_NSHIFT        16
#define IN_CLASSB_HOST          (0xffffffff & ~IN_CLASSB_NET)
#define IN_CLASSB_MAX           65536
#define IN_CLASSC(a)            ((((in_addr_t)(a)) & 0xe0000000) == 0xc0000000)
#define IN_CLASSC_NET           0xffffff00
#define IN_CLASSC_NSHIFT        8
#define IN_CLASSC_HOST          (0xffffffff & ~IN_CLASSC_NET)
#define IN_CLASSD(a)            ((((in_addr_t)(a)) & 0xf0000000) == 0xe0000000)
#define IN_MULTICAST(a)         IN_CLASSD(a)
#define IN_EXPERIMENTAL(a)      ((((in_addr_t)(a)) & 0xe0000000) == 0xe0000000)
#define IN_BADCLASS(a)          ((((in_addr_t)(a)) & 0xf0000000) == 0xf0000000)

#define IN_LOOPBACKNET 127

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
