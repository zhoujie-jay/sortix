/*
 * Copyright (c) 2011, 2012, 2013, 2014 Jonas 'Sortie' Termansen.
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
 * sys/types.h
 * Data types.
 */

#ifndef INCLUDE_SYS_TYPES_H
#define INCLUDE_SYS_TYPES_H

#include <sys/cdefs.h>

#include <sys/__/types.h>
#if __STDC_HOSTED__
#include <__/pthread.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __blkcnt_t_defined
#define __blkcnt_t_defined
typedef __blkcnt_t blkcnt_t;
#endif
#if __USE_SORTIX
#define BLKCNT_C(c) __BLKCNT_C(c)
#define BLKCNT_MIN __BLKCNT_MIN
#define BLKCNT_MAX __BLKCNT_MAX
#define PRIdBLKCNT __PRIdBLKCNT
#define PRIiBLKCNT __PRIiBLKCNT
#define PRIoBLKCNT __PRIoBLKCNT
#define PRIuBLKCNT __PRIuBLKCNT
#define PRIxBLKCNT __PRIxBLKCNT
#define PRIXBLKCNT __PRIXBLKCNT
#define SCNdBLKCNT __SCNdBLKCNT
#define SCNiBLKCNT __SCNiBLKCNT
#define SCNoBLKCNT __SCNoBLKCNT
#define SCNuBLKCNT __SCNuBLKCNT
#define SCNxBLKCNT __SCNxBLKCNT
#endif

#ifndef __blksize_t_defined
#define __blksize_t_defined
typedef __blksize_t blksize_t;
#endif
#if __USE_SORTIX
#define BLKSIZE_C(c) __BLKSIZE_C(c)
#define BLKSIZE_MIN __BLKSIZE_MIN
#define BLKSIZE_MAX __BLKSIZE_MAX
#define PRIdBLKSIZE __PRIdBLKSIZE
#define PRIiBLKSIZE __PRIiBLKSIZE
#define PRIoBLKSIZE __PRIoBLKSIZE
#define PRIuBLKSIZE __PRIuBLKSIZE
#define PRIxBLKSIZE __PRIxBLKSIZE
#define PRIXBLKSIZE __PRIXBLKSIZE
#define SCNdBLKSIZE __SCNdBLKSIZE
#define SCNiBLKSIZE __SCNiBLKSIZE
#define SCNoBLKSIZE __SCNoBLKSIZE
#define SCNuBLKSIZE __SCNuBLKSIZE
#define SCNxBLKSIZE __SCNxBLKSIZE
#endif

#ifndef __clock_t_defined
#define __clock_t_defined
typedef __clock_t clock_t;
#endif
#if __USE_SORTIX
#define CLOCK_C(c) __CLOCK_C(c)
#define CLOCK_MIN __CLOCK_MIN
#define CLOCK_MAX __CLOCK_MAX
#define PRIdCLOCK __PRIdCLOCK
#define PRIiCLOCK __PRIiCLOCK
#define PRIoCLOCK __PRIoCLOCK
#define PRIuCLOCK __PRIuCLOCK
#define PRIxCLOCK __PRIxCLOCK
#define PRIXCLOCK __PRIXCLOCK
#define SCNdCLOCK __SCNdCLOCK
#define SCNiCLOCK __SCNiCLOCK
#define SCNoCLOCK __SCNoCLOCK
#define SCNuCLOCK __SCNuCLOCK
#define SCNxCLOCK __SCNxCLOCK
#endif

#ifndef __clockid_t_defined
#define __clockid_t_defined
typedef __clockid_t clockid_t;
#endif
#if __USE_SORTIX
#define CLOCKID_C(c) __CLOCKID_C(c)
#define CLOCKID_MIN __CLOCKID_MIN
#define CLOCKID_MAX __CLOCKID_MAX
#define PRIdCLOCKID __PRIdCLOCKID
#define PRIiCLOCKID __PRIiCLOCKID
#define PRIoCLOCKID __PRIoCLOCKID
#define PRIuCLOCKID __PRIuCLOCKID
#define PRIxCLOCKID __PRIxCLOCKID
#define PRIXCLOCKID __PRIXCLOCKID
#define SCNdCLOCKID __SCNdCLOCKID
#define SCNiCLOCKID __SCNiCLOCKID
#define SCNoCLOCKID __SCNoCLOCKID
#define SCNuCLOCKID __SCNuCLOCKID
#define SCNxCLOCKID __SCNxCLOCKID
#endif

#ifndef __dev_t_defined
#define __dev_t_defined
typedef __dev_t dev_t;
#endif
#if __USE_SORTIX
#define DEV_C(c) __DEV_C(c)
#define DEV_MIN __DEV_MIN
#define DEV_MAX __DEV_MAX
#define PRIdDEV __PRIdDEV
#define PRIiDEV __PRIiDEV
#define PRIoDEV __PRIoDEV
#define PRIuDEV __PRIuDEV
#define PRIxDEV __PRIxDEV
#define PRIXDEV __PRIXDEV
#define SCNdDEV __SCNdDEV
#define SCNiDEV __SCNiDEV
#define SCNoDEV __SCNoDEV
#define SCNuDEV __SCNuDEV
#define SCNxDEV __SCNxDEV
#endif

#ifndef __fsblkcnt_t_defined
#define __fsblkcnt_t_defined
typedef __fsblkcnt_t fsblkcnt_t;
#endif
#if __USE_SORTIX
#define FSBLKCNT_C(c) __FSBLKCNT_C(c)
#define FSBLKCNT_MIN __FSBLKCNT_MIN
#define FSBLKCNT_MAX __FSBLKCNT_MAX
#define PRIdFSBLKCNT __PRIdFSBLKCNT
#define PRIiFSBLKCNT __PRIiFSBLKCNT
#define PRIoFSBLKCNT __PRIoFSBLKCNT
#define PRIuFSBLKCNT __PRIuFSBLKCNT
#define PRIxFSBLKCNT __PRIxFSBLKCNT
#define PRIXFSBLKCNT __PRIXFSBLKCNT
#define SCNdFSBLKCNT __SCNdFSBLKCNT
#define SCNiFSBLKCNT __SCNiFSBLKCNT
#define SCNoFSBLKCNT __SCNoFSBLKCNT
#define SCNuFSBLKCNT __SCNuFSBLKCNT
#define SCNxFSBLKCNT __SCNxFSBLKCNT
#endif

#ifndef __fsfilcnt_t_defined
#define __fsfilcnt_t_defined
typedef __fsfilcnt_t fsfilcnt_t;
#endif
#if __USE_SORTIX
#define FSFILCNT_C(c) __FSFILCNT_C(c)
#define FSFILCNT_MIN __FSFILCNT_MIN
#define FSFILCNT_MAX __FSFILCNT_MAX
#define PRIdFSFILCNT __PRIdFSFILCNT
#define PRIiFSFILCNT __PRIiFSFILCNT
#define PRIoFSFILCNT __PRIoFSFILCNT
#define PRIuFSFILCNT __PRIuFSFILCNT
#define PRIxFSFILCNT __PRIxFSFILCNT
#define PRIXFSFILCNT __PRIXFSFILCNT
#define SCNdFSFILCNT __SCNdFSFILCNT
#define SCNiFSFILCNT __SCNiFSFILCNT
#define SCNoFSFILCNT __SCNoFSFILCNT
#define SCNuFSFILCNT __SCNuFSFILCNT
#define SCNxFSFILCNT __SCNxFSFILCNT
#endif

#ifndef __gid_t_defined
#define __gid_t_defined
typedef __gid_t gid_t;
#endif
#if __USE_SORTIX
#define GID_C(c) __GID_C(c)
#define GID_MIN __GID_MIN
#define GID_MAX __GID_MAX
#define PRIdGID __PRIdGID
#define PRIiGID __PRIiGID
#define PRIoGID __PRIoGID
#define PRIuGID __PRIuGID
#define PRIxGID __PRIxGID
#define PRIXGID __PRIXGID
#define SCNdGID __SCNdGID
#define SCNiGID __SCNiGID
#define SCNoGID __SCNoGID
#define SCNuGID __SCNuGID
#define SCNxGID __SCNxGID
#endif

#ifndef __id_t_defined
#define __id_t_defined
typedef __id_t id_t;
#endif
#if __USE_SORTIX
#define ID_C(c) __ID_C(c)
#define ID_MIN __ID_MIN
#define ID_MAX __ID_MAX
#define PRIdID __PRIdID
#define PRIiID __PRIiID
#define PRIoID __PRIoID
#define PRIuID __PRIuID
#define PRIxID __PRIxID
#define PRIXID __PRIXID
#define SCNdID __SCNdID
#define SCNiID __SCNiID
#define SCNoID __SCNoID
#define SCNuID __SCNuID
#define SCNxID __SCNxID
#endif

#ifndef __ino_t_defined
#define __ino_t_defined
typedef __ino_t ino_t;
#endif
#if __USE_SORTIX
#define INO_C(c) __INO_C(c)
#define INO_MIN __INO_MIN
#define INO_MAX __INO_MAX
#define PRIdINO __PRIdINO
#define PRIiINO __PRIiINO
#define PRIoINO __PRIoINO
#define PRIuINO __PRIuINO
#define PRIxINO __PRIxINO
#define PRIXINO __PRIXINO
#define SCNdINO __SCNdINO
#define SCNiINO __SCNiINO
#define SCNoINO __SCNoINO
#define SCNuINO __SCNuINO
#define SCNxINO __SCNxINO
#endif

/* TODO: key_t */

#ifndef __mode_t_defined
#define __mode_t_defined
typedef __mode_t mode_t;
#endif
#if __USE_SORTIX
#define MODE_C(c) __MODE_C(c)
#define MODE_MIN __MODE_MIN
#define MODE_MAX __MODE_MAX
#define PRIdMODE __PRIdMODE
#define PRIiMODE __PRIiMODE
#define PRIoMODE __PRIoMODE
#define PRIuMODE __PRIuMODE
#define PRIxMODE __PRIxMODE
#define PRIXMODE __PRIXMODE
#define SCNdMODE __SCNdMODE
#define SCNiMODE __SCNiMODE
#define SCNoMODE __SCNoMODE
#define SCNuMODE __SCNuMODE
#define SCNxMODE __SCNxMODE
#endif

#ifndef __nlink_t_defined
#define __nlink_t_defined
typedef __nlink_t nlink_t;
#endif
#if __USE_SORTIX
#define NLINK_C(c) __NLINK_C(c)
#define NLINK_MIN __NLINK_MIN
#define NLINK_MAX __NLINK_MAX
#define PRIdNLINK __PRIdNLINK
#define PRIiNLINK __PRIiNLINK
#define PRIoNLINK __PRIoNLINK
#define PRIuNLINK __PRIuNLINK
#define PRIxNLINK __PRIxNLINK
#define PRIXNLINK __PRIXNLINK
#define SCNdNLINK __SCNdNLINK
#define SCNiNLINK __SCNiNLINK
#define SCNoNLINK __SCNoNLINK
#define SCNuNLINK __SCNuNLINK
#define SCNxNLINK __SCNxNLINK
#endif

#ifndef __off_t_defined
#define __off_t_defined
typedef __off_t off_t;
#endif
#if __USE_SORTIX
#define OFF_C(c) __OFF_C(c)
#define OFF_MIN __OFF_MIN
#define OFF_MAX __OFF_MAX
#define PRIdOFF __PRIdOFF
#define PRIiOFF __PRIiOFF
#define PRIoOFF __PRIoOFF
#define PRIuOFF __PRIuOFF
#define PRIxOFF __PRIxOFF
#define PRIXOFF __PRIXOFF
#define SCNdOFF __SCNdOFF
#define SCNiOFF __SCNiOFF
#define SCNoOFF __SCNoOFF
#define SCNuOFF __SCNuOFF
#define SCNxOFF __SCNxOFF
#endif

#ifndef __pid_t_defined
#define __pid_t_defined
typedef __pid_t pid_t;
#endif
#if __USE_SORTIX
#define PID_C(c) __PID_C(c)
#define PID_MIN __PID_MIN
#define PID_MAX __PID_MAX
#define PRIdPID __PRIdPID
#define PRIiPID __PRIiPID
#define PRIoPID __PRIoPID
#define PRIuPID __PRIuPID
#define PRIxPID __PRIxPID
#define PRIXPID __PRIXPID
#define SCNdPID __SCNdPID
#define SCNiPID __SCNiPID
#define SCNoPID __SCNoPID
#define SCNuPID __SCNuPID
#define SCNxPID __SCNxPID
#endif

#ifndef __size_t_defined
#define __size_t_defined
#define __need_size_t
#include <stddef.h>
#endif
#if __USE_SORTIX
#define SOCKLEN_C(c) __SOCKLEN_C(c)
#define SOCKLEN_MIN __SOCKLEN_MIN
#define SOCKLEN_MAX __SOCKLEN_MAX
#define PRIdSOCKLEN __PRIdSOCKLEN
#define PRIiSOCKLEN __PRIiSOCKLEN
#define PRIoSOCKLEN __PRIoSOCKLEN
#define PRIuSOCKLEN __PRIuSOCKLEN
#define PRIxSOCKLEN __PRIxSOCKLEN
#define PRIXSOCKLEN __PRIXSOCKLEN
#define SCNdSOCKLEN __SCNdSOCKLEN
#define SCNiSOCKLEN __SCNiSOCKLEN
#define SCNoSOCKLEN __SCNoSOCKLEN
#define SCNuSOCKLEN __SCNuSOCKLEN
#define SCNxSOCKLEN __SCNxSOCKLEN
#endif

#ifndef __ssize_t_defined
#define __ssize_t_defined
typedef __ssize_t ssize_t;
#endif
#if __USE_SORTIX
#define SSIZE_C(c) __SSIZE_C(c)
#define SSIZE_MIN __SSIZE_MIN
#if !defined(SSIZE_MAX) /* Also in <limits.h> */
#define SSIZE_MAX __SSIZE_MAX
#endif
#define PRIdSSIZE __PRIdSSIZE
#define PRIiSSIZE __PRIiSSIZE
#define PRIoSSIZE __PRIoSSIZE
#define PRIuSSIZE __PRIuSSIZE
#define PRIxSSIZE __PRIxSSIZE
#define PRIXSSIZE __PRIXSSIZE
#define SCNdSSIZE __SCNdSSIZE
#define SCNiSSIZE __SCNiSSIZE
#define SCNoSSIZE __SCNoSSIZE
#define SCNuSSIZE __SCNuSSIZE
#define SCNxSSIZE __SCNxSSIZE
#endif

#ifndef __suseconds_t_defined
#define __suseconds_t_defined
typedef __suseconds_t suseconds_t;
#endif
#if __USE_SORTIX
#define SUSECONDS_C(c) __SUSECONDS_C(c)
#define SUSECONDS_MIN __SUSECONDS_MIN
#define SUSECONDS_MAX __SUSECONDS_MAX
#define PRIdSUSECONDS __PRIdSUSECONDS
#define PRIiSUSECONDS __PRIiSUSECONDS
#define PRIoSUSECONDS __PRIoSUSECONDS
#define PRIuSUSECONDS __PRIuSUSECONDS
#define PRIxSUSECONDS __PRIxSUSECONDS
#define PRIXSUSECONDS __PRIXSUSECONDS
#define SCNdSUSECONDS __SCNdSUSECONDS
#define SCNiSUSECONDS __SCNiSUSECONDS
#define SCNoSUSECONDS __SCNoSUSECONDS
#define SCNuSUSECONDS __SCNuSUSECONDS
#define SCNxSUSECONDS __SCNxSUSECONDS
#endif

#ifndef __time_t_defined
#define __time_t_defined
typedef __time_t time_t;
#endif
#if __USE_SORTIX
#define TIME_C(c) __TIME_C(c)
#define TIME_MIN __TIME_MIN
#define TIME_MAX __TIME_MAX
#define PRIdTIME __PRIdTIME
#define PRIiTIME __PRIiTIME
#define PRIoTIME __PRIoTIME
#define PRIuTIME __PRIuTIME
#define PRIxTIME __PRIxTIME
#define PRIXTIME __PRIXTIME
#define SCNdTIME __SCNdTIME
#define SCNiTIME __SCNiTIME
#define SCNoTIME __SCNoTIME
#define SCNuTIME __SCNuTIME
#define SCNxTIME __SCNxTIME
#endif

#ifndef __timer_t_defined
#define __timer_t_defined
typedef __timer_t timer_t;
#endif
#if __USE_SORTIX
#define TIMER_C(c) __TIMER_C(c)
#define TIMER_MIN __TIMER_MIN
/* TODO: TIMER_MAX here conflicts with POSIX TIMER_MAX in <limits.h>. */
#define TIMER_MAX __TIMER_MAX
#define PRIdTIMER __PRIdTIMER
#define PRIiTIMER __PRIiTIMER
#define PRIoTIMER __PRIoTIMER
#define PRIuTIMER __PRIuTIMER
#define PRIxTIMER __PRIxTIMER
#define PRIXTIMER __PRIXTIMER
#define SCNdTIMER __SCNdTIMER
#define SCNiTIMER __SCNiTIMER
#define SCNoTIMER __SCNoTIMER
#define SCNuTIMER __SCNuTIMER
#define SCNxTIMER __SCNxTIMER
#endif

/* TODO: trace*_t */

#ifndef __uid_t_defined
#define __uid_t_defined
typedef __uid_t uid_t;
#endif
#if __USE_SORTIX
#define UID_C(c) __UID_C(c)
#define UID_MIN __UID_MIN
#define UID_MAX __UID_MAX
#define PRIdUID __PRIdUID
#define PRIiUID __PRIiUID
#define PRIoUID __PRIoUID
#define PRIuUID __PRIuUID
#define PRIxUID __PRIxUID
#define PRIXUID __PRIXUID
#define SCNdUID __SCNdUID
#define SCNiUID __SCNiUID
#define SCNoUID __SCNoUID
#define SCNuUID __SCNuUID
#define SCNxUID __SCNxUID
#endif

#if __STDC_HOSTED__

#ifndef __pthread_attr_t_defined
#define __pthread_attr_t_defined
typedef __pthread_attr_t pthread_attr_t;
#endif

#ifndef __pthread_barrier_t_defined
#define __pthread_barrier_t_defined
typedef __pthread_barrier_t pthread_barrier_t;
#endif

#ifndef __pthread_barrierattr_t_defined
#define __pthread_barrierattr_t_defined
typedef __pthread_barrierattr_t pthread_barrierattr_t;
#endif

#ifndef __pthread_cond_t_defined
#define __pthread_cond_t_defined
typedef __pthread_cond_t pthread_cond_t;
#endif

#ifndef __pthread_condattr_t_defined
#define __pthread_condattr_t_defined
typedef __pthread_condattr_t pthread_condattr_t;
#endif

#ifndef __pthread_key_t_defined
#define __pthread_key_t_defined
typedef __pthread_key_t pthread_key_t;
#endif

#ifndef __pthread_mutex_t_defined
#define __pthread_mutex_t_defined
typedef __pthread_mutex_t pthread_mutex_t;
#endif

#ifndef __pthread_mutexattr_t_defined
#define __pthread_mutexattr_t_defined
typedef __pthread_mutexattr_t pthread_mutexattr_t;
#endif

#ifndef __pthread_once_t_defined
#define __pthread_once_t_defined
typedef __pthread_once_t pthread_once_t;
#endif

#ifndef __pthread_rwlock_t_defined
#define __pthread_rwlock_t_defined
typedef __pthread_rwlock_t pthread_rwlock_t;
#endif

#ifndef __pthread_rwlockattr_t_defined
#define __pthread_rwlockattr_t_defined
typedef __pthread_rwlockattr_t pthread_rwlockattr_t;
#endif

#ifndef __pthread_spinlock_t_defined
#define __pthread_spinlock_t_defined
typedef __pthread_spinlock_t pthread_spinlock_t;
#endif

#ifndef __pthread_t_defined
#define __pthread_t_defined
typedef __pthread_t pthread_t;
#endif

#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
