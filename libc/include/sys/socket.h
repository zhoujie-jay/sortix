/*
 * Copyright (c) 2013, 2014 Jonas 'Sortie' Termansen.
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
 * sys/socket.h
 * Main sockets header.
 */

#ifndef _SYS_SOCKET_H
#define _SYS_SOCKET_H 1

#include <sys/cdefs.h>

#include <sys/__/types.h>

#include <sortix/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __socklen_t_defined
#define __socklen_t_defined
typedef __socklen_t socklen_t;
#endif

#ifndef __size_t_defined
#define __size_t_defined
#define __need_size_t
#include <stddef.h>
#endif

#ifndef __ssize_t_defined
#define __ssize_t_defined
typedef __ssize_t ssize_t;
#endif

#ifndef __sa_family_t_defined
#define __sa_family_t_defined
typedef unsigned short int sa_family_t;
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#include <sortix/uio.h>

#ifdef __cplusplus
extern "C" {
#endif

struct sockaddr
{
	sa_family_t sa_family;
	char sa_data[16 - sizeof(sa_family_t)];
};

#define __ss_aligntype unsigned long int
#define _SS_SIZE 128
#define _SS_PADSIZE (_SS_SIZE - (2 * sizeof (__ss_aligntype)))
struct sockaddr_storage
{
	sa_family_t ss_family;
	__ss_aligntype __ss_align;
	char __ss_padding[_SS_PADSIZE];
};

struct msghdr
{
	void* msg_name;
	socklen_t msg_namelen;
	struct iovec* msg_iov;
	int msg_iovlen;
	void* msg_control;
	socklen_t msg_controllen;
	int msg_flags;
};

struct cmsghdr
{
	socklen_t cmsg_len;
	int cmsg_level;
	int cmsg_type;
};

#define SCM_RIGHTS 1

/* TODO: CMSG_DATA(cmsg) */
/* TODO: CMSG_NXTHDR(cmsg) */
/* TODO: CMSH_FIRSTHDR(cmsg) */

struct linger
{
	int l_onoff;
	int l_linger;
};

#define SOL_SOCKET 0
/* #define IPPROTO_* constants follow numerically. */

/* Options at the SOL_SOCKET socket level. */
#define SO_ACCEPTCONN 1
#define SO_BROADCAST 2
#define SO_DEBUG 3
#define SO_DONTROUTE 4
#define SO_ERROR 5
#define SO_KEEPALIVE 6
#define SO_LINGER 7
#define SO_OOBINLINE 8
#define SO_RCVBUF 9
#define SO_RCVLOWAT 10
#define SO_RCVTIMEO 11
#define SO_REUSEADDR 12
#define SO_SNDBUF 13
#define SO_SNDLOWAT 14
#define SO_SNDTIMEO 15
#define SO_TYPE 16

#define SOMAXCONN 5

#define MSG_CTRUNC (1<<0)
#define MSG_DONTROUTE (1<<1)
#define MSG_EOR (1<<2)
#define MSG_OOB (1<<3)
#define MSG_NOSIGNAL (1<<4)
#define MSG_PEEK (1<<5)
#define MSG_TRUNC (1<<6)
#define MSG_WAITALL (1<<7)
#define MSG_DONTWAIT (1<<8)
#define MSG_CMSG_CLOEXEC (1<<9)

#define AF_UNSPEC 0
#define AF_INET 1
#define AF_INET6 2
#define AF_UNIX 3

/* TODO: POSIX doesn't specifiy these and it is a bit of a BSD-ism, but they
         appear to be used in some software (such as GNU wget). */
#define PF_UNSPEC AF_UNSPEC
#define PF_INET AF_INET
#define PF_INET6 AF_INET6
#define PF_UNIX AF_UNIX

#define AF_LOCAL AF_UNIX
#define PF_LOCAL PF_UNIX

/* TODO: Nicely wrap this in an enum, as in glibc's header? */
#define SHUT_RD (1 << 0)
#define SHUT_WR (1 << 1)
#define SHUT_RDWR (SHUT_RD | SHUT_WR)

int accept4(int, struct sockaddr* __restrict, socklen_t* __restrict, int);
int accept(int, struct sockaddr* __restrict, socklen_t* __restrict);
int bind(int, const struct sockaddr*, socklen_t);
int connect(int, const struct sockaddr*, socklen_t);
int getpeername(int, struct sockaddr* __restrict, socklen_t* __restrict);
int getsockname(int, struct sockaddr* __restrict, socklen_t* __restrict);
int getsockopt(int, int, int, void* __restrict, socklen_t* __restrict);
int listen(int, int);
ssize_t recv(int, void*, size_t, int);
ssize_t recvfrom(int, void* __restrict, size_t, int,
        struct sockaddr* __restrict, socklen_t* __restrict);
ssize_t recvmsg(int, struct msghdr*, int);
ssize_t send(int, const void*, size_t, int);
ssize_t sendmsg(int, const struct msghdr*, int);
ssize_t sendto(int, const void*, size_t, int, const struct sockaddr*, socklen_t);
int setsockopt(int, int, int, const void*, socklen_t);
int shutdown(int, int);
int sockatmark(int);
int socket(int, int, int);
int socketpair(int, int, int, int[2]);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
