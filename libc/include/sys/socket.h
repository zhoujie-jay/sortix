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

    sys/socket.h
    Main sockets header.

*******************************************************************************/

#ifndef _SYS_SOCKET_H
#define _SYS_SOCKET_H 1

#include <features.h>
/* TODO: #include <sys/uio.h> */
#include <sortix/socket.h>

__BEGIN_DECLS

@include(socklen_t.h)
@include(size_t.h)
@include(ssize_t.h)
@include(sa_family_t.h)

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

/* TODO: struct iovec from <sys/uio.h> */

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

#define SOL_SOCKET 1

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

/* TODO: Nicely wrap this in an enum, as in glibc's header? */
/* TODO: Should SHUT_RDWR = SHUT_RD | SHUT_WR? */
#define SHUT_RD 0
#define SHUT_RDWR 1
#define SHUT_WR 2

int accept4(int, struct sockaddr* restrict, socklen_t* restrict, int);
int accept(int, struct sockaddr* restrict, socklen_t* restrict);
int bind(int, const struct sockaddr*, socklen_t);
int connect(int, const struct sockaddr*, socklen_t);
int getpeername(int, struct sockaddr* restrict, socklen_t* restrict);
int getsockname(int, struct sockaddr* restrict, socklen_t* restrict);
int getsockopt(int, int, int, void* restrict, socklen_t* restrict);
int listen(int, int);
ssize_t recv(int, void*, size_t, int);
ssize_t recvfrom(int, void* restrict, size_t, int,
        struct sockaddr* restrict, socklen_t* restrict);
ssize_t recvmsg(int, struct msghdr*, int);
ssize_t send(int, const void*, size_t, int);
ssize_t sendmsg(int, const struct msghdr*, int);
ssize_t sendto(int, const void*, size_t, int, const struct sockaddr*, socklen_t);
int setsockopt(int, int, int, const void*, socklen_t);
int shutdown(int, int);
int sockatmark(int);
int socket(int, int, int);
int socketpair(int, int, int, int[2]);

__END_DECLS

#endif