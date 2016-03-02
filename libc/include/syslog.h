/*
 * Copyright (c) 2014 Jonas 'Sortie' Termansen.
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
 * syslog.h
 * System error logging facility.
 */

#ifndef INCLUDE_SYSLOG_H
#define INCLUDE_SYSLOG_H

#include <sys/cdefs.h>

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LOG_PRISHIFT 0
#define LOG_PRIBITS 3
#define LOG_FACSHIFT LOG_PRIBITS
#define LOG_FACBITS (31 - LOG_PRIBITS)

#define LOG_PRIMASK (((1 << LOG_PRIBITS) - 1) << LOG_PRISHIFT)
#define LOG_FACMASK (((1 << LOG_FACBITS) - 1) << LOG_FACSHIFT)
#define LOG_PRI(x) ((x) >> LOG_PRISHIFT & LOG_PRIMASK)
#define LOG_FAC(x) ((x) >> LOG_FACSHIFT & LOG_FACMASK)

#define LOG_MAKEPRI(fac, pri) ((fac) << LOG_FACSHIFT | (pri) << LOG_PRISHIFT)

#define LOG_EMERG 0
#define LOG_ALERT 1
#define LOG_CRIT 2
#define LOG_ERR 3
#define LOG_WARNING 4
#define LOG_NOTICE 5
#define LOG_INFO 6
#define LOG_DEBUG 7

#define LOG_MASK(pri) (1 << ((pri) >> LOG_PRISHIFT))
#define LOG_UPTO(pri) ((1 << (((pri) >> LOG_PRISHIFT) + 1)) - 1)

#define LOG_KERN (0 << LOG_FACSHIFT)
#define LOG_USER (1 << LOG_FACSHIFT)
#define LOG_MAIL (2 << LOG_FACSHIFT)
#define LOG_DAEMON (3 << LOG_FACSHIFT)
#define LOG_AUTH (4 << LOG_FACSHIFT)
#define LOG_SYSLOG (5 << LOG_FACSHIFT)
#define LOG_LPR (6 << LOG_FACSHIFT)
#define LOG_NEWS (7 << LOG_FACSHIFT)
#define LOG_UUCP (8 << LOG_FACSHIFT)
#define LOG_CRON (9 << LOG_FACSHIFT)
#define LOG_AUTHPRIV (10 << LOG_FACSHIFT)
#define LOG_FTP (11 << LOG_FACSHIFT)
/* 12 through 15 are reserved */
#define LOG_LOCAL0 (16 << LOG_FACSHIFT)
#define LOG_LOCAL1 (17 << LOG_FACSHIFT)
#define LOG_LOCAL2 (18 << LOG_FACSHIFT)
#define LOG_LOCAL3 (19 << LOG_FACSHIFT)
#define LOG_LOCAL4 (20 << LOG_FACSHIFT)
#define LOG_LOCAL5 (21 << LOG_FACSHIFT)
#define LOG_LOCAL6 (22 << LOG_FACSHIFT)
#define LOG_LOCAL7 (23 << LOG_FACSHIFT)
#define LOG_NFACILITIES 24

#define LOG_CONS (1 << 0)
#define LOG_NDELAY (1 << 1)
#define LOG_NOWAIT (1 << 2)
#define LOG_ODELAY (1 << 3)
#define LOG_PERROR (1 << 4)
#define LOG_PID (1 << 5)

#if defined(__is_sortix_libc)
extern char* __syslog_identity;
extern int __syslog_facility;
extern int __syslog_fd;
extern int __syslog_mask;
extern int __syslog_option;
#endif

void closelog(void);
int connectlog(void);
void openlog(const char*, int, int);
int setlogmask(int);
__attribute__ ((__format__ (__printf__, 2, 3)))
void syslog(int, const char*, ...);
__attribute__ ((__format__ (__printf__, 2, 0)))
void vsyslog(int, const char*, va_list);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
