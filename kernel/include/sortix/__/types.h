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
 * sortix/__/types.h
 * Data types.
 */

#ifndef INCLUDE_SORTIX____TYPES_H
#define INCLUDE_SORTIX____TYPES_H

#include <sys/cdefs.h>

#include <__/limits.h>
#include <__/stdint.h>
#include <__/wordsize.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef __intmax_t __blkcnt_t;
/*#define __BLKCNT_UNSIGNED*/
#define __BLKCNT_C(c) __INTMAX_C(c)
#define __BLKCNT_MIN __INTMAX_MIN
#define __BLKCNT_MAX __INTMAX_MAX
#define __PRIdBLKCNT __PRIdMAX
#define __PRIiBLKCNT __PRIiMAX
#define __PRIoBLKCNT __PRIoMAX
#define __PRIuBLKCNT __PRIuMAX
#define __PRIxBLKCNT __PRIxMAX
#define __PRIXBLKCNT __PRIXMAX
#define __SCNdBLKCNT __SCNdMAX
#define __SCNiBLKCNT __SCNiMAX
#define __SCNoBLKCNT __SCNoMAX
#define __SCNuBLKCNT __SCNuMAX
#define __SCNxBLKCNT __SCNxMAX

typedef __intmax_t __blksize_t;
/*#define __BLKSIZE_UNSIGNED*/
#define __BLKSIZE_C(c) __INTMAX_C(c)
#define __BLKSIZE_MIN __INTMAX_MIN
#define __BLKSIZE_MAX __INTMAX_MAX
#define __PRIdBLKSIZE __PRIdMAX
#define __PRIiBLKSIZE __PRIiMAX
#define __PRIoBLKSIZE __PRIoMAX
#define __PRIuBLKSIZE __PRIuMAX
#define __PRIxBLKSIZE __PRIxMAX
#define __PRIXBLKSIZE __PRIXMAX
#define __SCNdBLKSIZE __SCNdMAX
#define __SCNiBLKSIZE __SCNiMAX
#define __SCNoBLKSIZE __SCNoMAX
#define __SCNuBLKSIZE __SCNuMAX
#define __SCNxBLKSIZE __SCNxMAX

typedef long __clock_t;
/*#define __CLOCK_UNSIGNED*/
#define __CLOCK_C(c) c ## L
#define __CLOCK_MIN __LONG_MIN
#define __CLOCK_MAX __LONG_MAX
#define __PRIdCLOCK "ld"
#define __PRIiCLOCK "li"
#define __PRIoCLOCK "lo"
#define __PRIuCLOCK "lu"
#define __PRIxCLOCK "lx"
#define __PRIXCLOCK "lX"
#define __SCNdCLOCK "ld"
#define __SCNiCLOCK "li"
#define __SCNoCLOCK "lo"
#define __SCNuCLOCK "lu"
#define __SCNxCLOCK "lx"

typedef int __clockid_t;
/*#define __CLOCKID_UNSIGNED*/
#define __CLOCKID_C(c) c
#define __CLOCKID_MIN __INT_MIN
#define __CLOCKID_MAX __INT_MAX
#define __PRIdCLOCKID "d"
#define __PRIiCLOCKID "i"
#define __PRIoCLOCKID "o"
#define __PRIuCLOCKID "u"
#define __PRIxCLOCKID "x"
#define __PRIXCLOCKID "X"
#define __SCNdCLOCKID "d"
#define __SCNiCLOCKID "i"
#define __SCNoCLOCKID "o"
#define __SCNuCLOCKID "u"
#define __SCNxCLOCKID "x"

typedef __uintptr_t __dev_t;
#define __DEV_UNSIGNED
#define __DEV_C(c) __UINTPTR_C(c)
#define __DEV_MIN 0
#define __DEV_MAX __UINTPTR_MAX
#define __PRIdDEV __PRIdPTR
#define __PRIiDEV __PRIiPTR
#define __PRIoDEV __PRIoPTR
#define __PRIuDEV __PRIuPTR
#define __PRIxDEV __PRIxPTR
#define __PRIXDEV __PRIXPTR
#define __SCNdDEV __SCNdPTR
#define __SCNiDEV __SCNiPTR
#define __SCNoDEV __SCNoPTR
#define __SCNuDEV __SCNuPTR
#define __SCNxDEV __SCNxPTR

typedef __intmax_t __fsblkcnt_t;
/*#define __FSBLKCNT_UNSIGNED*/
#define __FSBLKCNT_C(c) __INTMAX_C(c)
#define __FSBLKCNT_MIN __INTMAX_MIN
#define __FSBLKCNT_MAX __INTMAX_MAX
#define __PRIdFSBLKCNT __PRIdMAX
#define __PRIiFSBLKCNT __PRIiMAX
#define __PRIoFSBLKCNT __PRIoMAX
#define __PRIuFSBLKCNT __PRIuMAX
#define __PRIxFSBLKCNT __PRIxMAX
#define __PRIXFSBLKCNT __PRIXMAX
#define __SCNdFSBLKCNT __SCNdMAX
#define __SCNiFSBLKCNT __SCNiMAX
#define __SCNoFSBLKCNT __SCNoMAX
#define __SCNuFSBLKCNT __SCNuMAX
#define __SCNxFSBLKCNT __SCNxMAX

typedef __intmax_t __fsfilcnt_t;
/*#define __FSFILCNT_UNSIGNED*/
#define __FSFILCNT_C(c) __INTMAX_C(c)
#define __FSFILCNT_MIN __INTMAX_MIN
#define __FSFILCNT_MAX __INTMAX_MAX
#define __PRIdFSFILCNT __PRIdMAX
#define __PRIiFSFILCNT __PRIiMAX
#define __PRIoFSFILCNT __PRIoMAX
#define __PRIuFSFILCNT __PRIuMAX
#define __PRIxFSFILCNT __PRIxMAX
#define __PRIXFSFILCNT __PRIXMAX
#define __SCNdFSFILCNT __SCNdMAX
#define __SCNiFSFILCNT __SCNiMAX
#define __SCNoFSFILCNT __SCNoMAX
#define __SCNuFSFILCNT __SCNuMAX
#define __SCNxFSFILCNT __SCNxMAX

typedef __uint64_t __gid_t;
#define __GID_UNSIGNED
#define __GID_C(c) __UINT64_C(c)
#define __GID_MIN 0
#define __GID_MAX __UINT64_MAX
#define __PRIdGID __PRId64
#define __PRIiGID __PRIi64
#define __PRIoGID __PRIo64
#define __PRIuGID __PRIu64
#define __PRIxGID __PRIx64
#define __PRIXGID __PRIX64
#define __SCNdGID __SCNd64
#define __SCNiGID __SCNi64
#define __SCNoGID __SCNo64
#define __SCNuGID __SCNu64
#define __SCNxGID __SCNx64

typedef __uint64_t __id_t;
#define __ID_UNSIGNED
#define __ID_C(c) __UINT64_C(c)
#define __ID_MIN 0
#define __ID_MAX __UINT64_MAX
#define __PRIdID __PRId64
#define __PRIiID __PRIi64
#define __PRIoID __PRIo64
#define __PRIuID __PRIu64
#define __PRIxID __PRIx64
#define __PRIXID __PRIX64
#define __SCNdID __SCNd64
#define __SCNiID __SCNi64
#define __SCNoID __SCNo64
#define __SCNuID __SCNu64
#define __SCNxID __SCNx64

typedef __uintmax_t __ino_t;
#define __INO_UNSIGNED
#define __INO_C(c) __INTMAX_C(c)
#define __INO_MIN __INTMAX_MIN
#define __INO_MAX __INTMAX_MAX
#define __PRIdINO __PRIdMAX
#define __PRIiINO __PRIiMAX
#define __PRIoINO __PRIoMAX
#define __PRIuINO __PRIuMAX
#define __PRIxINO __PRIxMAX
#define __PRIXINO __PRIXMAX
#define __SCNdINO __SCNdMAX
#define __SCNiINO __SCNiMAX
#define __SCNoINO __SCNoMAX
#define __SCNuINO __SCNuMAX
#define __SCNxINO __SCNxMAX

/* TODO: __key_t */

typedef unsigned int __mode_t;
#define __MODE_UNSIGNED
#define __MODE_C(c) c ## U
#define __MODE_MIN 0
#define __MODE_MAX __UINT_MAX
#define __PRIdMODE "d"
#define __PRIiMODE "i"
#define __PRIoMODE "o"
#define __PRIuMODE "u"
#define __PRIxMODE "x"
#define __PRIXMODE "X"
#define __SCNdMODE "d"
#define __SCNiMODE "i"
#define __SCNoMODE "o"
#define __SCNuMODE "u"
#define __SCNxMODE "x"

typedef unsigned int __nlink_t;
#define __NLINK_UNSIGNED
#define __NLINK_C(c) c ## U
#define __NLINK_MIN 0
#define __NLINK_MAX __UINT_MAX
#define __PRIdNLINK "d"
#define __PRIiNLINK "i"
#define __PRIoNLINK "o"
#define __PRIuNLINK "u"
#define __PRIxNLINK "x"
#define __PRIXNLINK "X"
#define __SCNdNLINK "d"
#define __SCNiNLINK "i"
#define __SCNoNLINK "o"
#define __SCNuNLINK "u"
#define __SCNxNLINK "x"

typedef __intmax_t __off_t;
/*#define __OFF_UNSIGNED*/
#define __OFF_C(c) __INTMAX_C(c)
#define __OFF_MIN __INTMAX_MIN
#define __OFF_MAX __INTMAX_MAX
#define __PRIdOFF __PRIdMAX
#define __PRIiOFF __PRIiMAX
#define __PRIoOFF __PRIoMAX
#define __PRIuOFF __PRIuMAX
#define __PRIxOFF __PRIxMAX
#define __PRIXOFF __PRIXMAX
#define __SCNdOFF __SCNdMAX
#define __SCNiOFF __SCNiMAX
#define __SCNoOFF __SCNoMAX
#define __SCNuOFF __SCNuMAX
#define __SCNxOFF __SCNxMAX

typedef __intptr_t __pid_t;
/*#define __PID_UNSIGNED*/
#define __PID_C(c) __INTPTR_C(c)
#define __PID_MIN __INTPTR_MIN
#define __PID_MAX __INTPTR_MAX
#define __PRIdPID __PRIdPTR
#define __PRIiPID __PRIiPTR
#define __PRIoPID __PRIoPTR
#define __PRIuPID __PRIuPTR
#define __PRIxPID __PRIxPTR
#define __PRIXPID __PRIXPTR
#define __SCNdPID __SCNdPTR
#define __SCNiPID __SCNiPTR
#define __SCNoPID __SCNoPTR
#define __SCNuPID __SCNuPTR
#define __SCNxPID __SCNxPTR

/* TODO: __size_t */

typedef __SIZE_TYPE__ __socklen_t;
#define __SOCKLEN_UNSIGNED
/* TODO: Get this from a __SIZE_C */
#if defined(__i386__)
#define __SOCKLEN_C(c) c ## u
#elif defined(__x86_64__)
#define __SOCKLEN_C(c) c ## ul
#else
#error "You need to provide __SOCKLEN_C for your platform"
#endif
#define __SOCKLEN_MIN 0
#define __SOCKLEN_MAX __SIZE_MAX__
#define __PRIdSOCKLEN "zd"
#define __PRIiSOCKLEN "zi"
#define __PRIoSOCKLEN "zo"
#define __PRIuSOCKLEN "zu"
#define __PRIxSOCKLEN "zx"
#define __PRIXSOCKLEN "zX"
#define __SCNdSOCKLEN "zd"
#define __SCNiSOCKLEN "zi"
#define __SCNoSOCKLEN "zo"
#define __SCNuSOCKLEN "zu"
#define __SCNxSOCKLEN "zx"

typedef __SSIZE_TYPE__ __ssize_t;
/*#define __SSIZE_UNSIGNED*/
/* TODO: Get this from a __SSIZE_C */
#if defined(__i386__)
#define __SSIZE_C(c) c
#elif defined(__x86_64__)
#define __SSIZE_C(c) c ## l
#else
#error "You need to provide __SSIZE_C for your platform"
#endif
#if defined(__i386__)
#define __SSIZE_MIN __INT_MIN
#define __SSIZE_MAX __INT_MAX
#elif defined(__x86_64__)
#define __SSIZE_MIN __LONG_MIN
#define __SSIZE_MAX __LONG_MAX
#else
#error "You need to provide __SSIZE_MIN and __SSIZE_MAX for your platform"
#endif
#define __PRIdSSIZE "zd"
#define __PRIiSSIZE "zi"
#define __PRIoSSIZE "zo"
#define __PRIuSSIZE "zu"
#define __PRIxSSIZE "zx"
#define __PRIXSSIZE "zX"
#define __SCNdSSIZE "zd"
#define __SCNiSSIZE "zi"
#define __SCNoSSIZE "zo"
#define __SCNuSSIZE "zu"
#define __SCNxSSIZE "zx"

typedef long __suseconds_t;
/*#define __SUSECONDS_UNSIGNED*/
#define __SUSECONDS_C(c) c ## L
#define __SUSECONDS_MIN __LONG_MIN
#define __SUSECONDS_MAX __LONG_MAX
#define __PRIdSUSECONDS "ld"
#define __PRIiSUSECONDS "li"
#define __PRIoSUSECONDS "lo"
#define __PRIuSUSECONDS "lu"
#define __PRIxSUSECONDS "lx"
#define __PRIXSUSECONDS "lX"
#define __SCNdSUSECONDS "ld"
#define __SCNiSUSECONDS "li"
#define __SCNoSUSECONDS "lo"
#define __SCNuSUSECONDS "lu"
#define __SCNxSUSECONDS "lx"

typedef __int64_t __time_t;
/*#define __TIME_UNSIGNED*/
#define __TIME_C(c) __INT64_C(c)
#define __TIME_MIN __INT64_MIN
#define __TIME_MAX __INT64_MAX
#define __PRIdTIME __PRId64
#define __PRIiTIME __PRIi64
#define __PRIoTIME __PRIo64
#define __PRIuTIME __PRIu64
#define __PRIxTIME __PRIx64
#define __PRIXTIME __PRIX64
#define __SCNdTIME __SCNd64
#define __SCNiTIME __SCNi64
#define __SCNoTIME __SCNo64
#define __SCNuTIME __SCNu64
#define __SCNxTIME __SCNx64

typedef __uintptr_t __timer_t;
#define __TIMER_UNSIGNED
#define __TIMER_C(c) __UINTPTR_C(c)
#define __TIMER_MIN 0
#define __TIMER_MAX __UINTPTR_MAX
#define __PRIdTIMER __PRIdPTR
#define __PRIiTIMER __PRIiPTR
#define __PRIoTIMER __PRIoPTR
#define __PRIuTIMER __PRIuPTR
#define __PRIxTIMER __PRIxPTR
#define __PRIXTIMER __PRIXPTR
#define __SCNdTIMER __SCNdPTR
#define __SCNiTIMER __SCNiPTR
#define __SCNoTIMER __SCNoPTR
#define __SCNuTIMER __SCNuPTR
#define __SCNxTIMER __SCNxPTR

/* TODO: trace*_t */

typedef __uint64_t __uid_t;
#define __UID_UNSIGNED
#define __UID_C(c) __UINT64_C(c)
#define __UID_MIN 0
#define __UID_MAX __UINT64_MAX
#define __PRIdUID __PRId64
#define __PRIiUID __PRIi64
#define __PRIoUID __PRIo64
#define __PRIuUID __PRIu64
#define __PRIxUID __PRIx64
#define __PRIXUID __PRIX64
#define __SCNdUID __SCNd64
#define __SCNiUID __SCNi64
#define __SCNoUID __SCNo64
#define __SCNuUID __SCNu64
#define __SCNxUID __SCNx64

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
