/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2014, 2015.

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

    unistd.h
    Standard symbolic constants and types.

*******************************************************************************/

#ifndef INCLUDE_UNISTD_H
#define INCLUDE_UNISTD_H

#include <sys/cdefs.h>

#include <sys/__/types.h>
#include <__/stdint.h>

#include <sortix/seek.h>
#include <sortix/unistd.h>

#if __USE_SORTIX
#include <sortix/exit.h>
#include <sortix/fork.h>
#include <sortix/timespec.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* If a POSIX revision was already decided by feature macros. */
#if __USE_POSIX
#define _POSIX_VERSION  __USE_POSIX /* C bindings */
#define _POSIX2_VERSION __USE_POSIX /* Shell utilities. */

/* The native API is based on POSIX 2008. */
#elif __USE_SORTIX
#define _POSIX_VERSION  200809L /* C bindings */
#define _POSIX2_VERSION 200809L /* Shell utilities. */

/* That's odd. This is a POSIX header, but the POSIX API is not visible. The
   best option is probably to just say we are the 1990 POSIX standard, since it
   is not protected by feature macros in this header. */
#else
#define _POSIX_VERSION  199009L /* C bindings */
#define _POSIX2_VERSION 199209L /* Shell utilities. */
#endif

/* If an X/OPEN revision was already decided by feature macros.
   TODO: Sortix refuses to implement stupid parts of the POSIX; the XSI option
         is usually a great clue something is stupid. */
#if 600 < __USE_XOPEN
#define _XOPEN_VERSION __USE_XOPEN
#elif 500 <= __USE_XOPEN
#define _XOPEN_VERSION 500
#else
#define _XOPEN_VERSION 4
#endif

/* #define _POSIX_ADVISORY_INFO 200809L
   TODO: Uncomment when posix_fadvise(), posix_fallocate(), posix_madvise(),
         posix_memalign() has been added. */
#define _POSIX_ASYNCHRONOUS_IO 200809L
#define _POSIX_BARRIERS 200809L
/* TODO: _POSIX_CHOWN_RESTRICTED - Decide when security policies are implemented. */
#define _POSIX_CLOCK_SELECTION 200809L
#define _POSIX_CPUTIME 200809L
#define _POSIX_FSYNC 200809L
#define _POSIX_IPV6 200809L
#define _POSIX_JOB_CONTROL
/*TODO: _POSIX_MEMLOCK - Research what this is. */
/*TODO: _POSIX_MEMLOCK_RANGE - Research what this is. */
#define _POSIX_MEMORY_PROTECTION 200809L
/*TODO: _POSIX_MESSAGE_PASSING - Research what this is. */
#define _POSIX_MONOTONIC_CLOCK 200809L
#define _POSIX_NO_TRUNC 1
/*TODO: _POSIX_PRIORITIZED_IO - Research what this is. */
/*TODO: _POSIX_PRIORITY_SCHEDULING - Research what this is. */
/*TODO: _POSIX_RAW_SOCKETS - Research what this is. */
#define _POSIX_READER_WRITER_LOCKS 200809L
#define _POSIX_REALTIME_SIGNALS 200809L
/* #define _POSIX_REGEXP 1
   TODO: Uncomment when regular expressions are implemented. */
/* #define _POSIX_SAVED_IDS 1
   TODO: Uncomment when saved ids are implemented. I forgot if they already are. */
#define _POSIX_SEMAPHORES 200809L
/*TODO: _POSIX_SHARED_MEMORY_OBJECTS - Research what this is. */
#define _POSIX_SHELL 1
/*TODO: _POSIX_SPAWN - Research what this is. */
#define _POSIX_SPIN_LOCKS 200809L
/*TODO: _POSIX_SPORADIC_SERVER - Research what this is. */
/*TODO: _POSIX_SYNCHRONIZED_IO - Research what this is. */
/*TODO: _POSIX_THREAD_ATTR_STACKADDR - Research what this is, cooperate with libpthread. */
/*TODO: _POSIX_THREAD_ATTR_STACKSIZE - Research what this is, cooperate with libpthread. */
#define _POSIX_THREAD_CPUTIME 200809L
/*TODO: _POSIX_THREAD_PRIO_INHERIT - Research what this is, cooperate with libpthread. */
/*TODO: _POSIX_THREAD_PRIO_PROTECT - Research what this is, cooperate with libpthread. */
/*TODO: _POSIX_THREAD_PRIORITY_SCHEDULING - Research what this is, cooperate with libpthread. */
/*TODO: _POSIX_THREAD_PROCESS_SHARED - Research what this is, cooperate with libpthread. */
/*TODO: _POSIX_THREAD_ROBUST_PRIO_INHERIT - Research what this is, cooperate with libpthread. */
/*TODO: _POSIX_THREAD_ROBUST_PRIO_PROTECT - Research what this is, cooperate with libpthread. */
#define _POSIX_THREAD_SAFE_FUNCTIONS 200809L
/*TODO: _POSIX_THREAD_SPORADIC_SERVER - Research what this is, cooperate with libpthread. */
#define _POSIX_THREADS 200809L
#define _POSIX_TIMEOUTS 200809L
#define _POSIX_TIMERS 200809L
/* TODO: _POSIX_TRACE (Obsolescent) - Research what this is. */
/* TODO: _POSIX_TRACE_EVENT_FILTER (Obsolescent) - Research what this is. */
/* TODO: _POSIX_TRACE_INHERIT (Obsolescent) - Research what this is. */
/* TODO: _POSIX_TRACE_LOG (Obsolescent) - Research what this is. */
/* TODO: TYPED_MEMORY_OBJECTS - Research what this is. */
/* TODO: _POSIX_V6_ILP32_OFF32 (Obsolescent) - Research what this is. */
/* TODO: _POSIX_V6_ILP32_OFFBIG (Obsolescent) - Research what this is. */
/* TODO: _POSIX_V6_LP64_OFF64 (Obsolescent) - Research what this is. */
/* TODO: _POSIX_V6_LPBIG_OFFBIG (Obsolescent) - Research what this is. */
/* TODO: _POSIX_V7_ILP32_OFF32 - Research what this is. */
/* TODO: _POSIX_V7_ILP32_OFFBIG - Research what this is. */
/* TODO: _POSIX_V7_LP64_OFF64 - Research what this is. */
/* TODO: _POSIX_V7_LPBIG_OFFBIG - Research what this is. */
#define _POSIX2_C_BIND _POSIX2_VERSION
#define _POSIX2_C_DEV _POSIX2_VERSION
#define _POSIX2_CHAR_TERM 1
/* TODO: _POSIX2_FORT_RUN - When fortran becomes supported. */
/* #define _POSIX2_LOCALEDEF __POSIX2_THIS_VERSION
   TODO: Uncomment when locales are implemented. */
/* TODO: _POSIX2_PBS (Obsolescent) - Research what this is. */
/* TODO: _POSIX2_PBS_ACCOUNTING (Obsolescent) - Research what this is. */
/* TODO: _POSIX2_PBS_CHECKPOINT (Obsolescent) - Research what this is. */
/* TODO: _POSIX2_PBS_LOCATE (Obsolescent) - Research what this is. */
/* TODO: _POSIX2_PBS_MESSAGE (Obsolescent) - Research what this is. */
/* TODO: _POSIX2_PBS_TRACK (Obsolescent) - Research what this is. */
/* TODO: _POSIX2_SW_DEV - Research what this is. (Define to _POSIX2_VERSION) */
/* #define _POSIX2_UPE 200809L
   TODO: Uncomment when bg, ex, fc, fg, jobs, more, talk, vi are implemented. */
/* TODO: _XOPEN_CRYPT - Research what this is. */
#define _XOPEN_ENH_I18N 1
#define _XOPEN_REALTIME 1
#define _XOPEN_REALTIME 1
/* TODO: _XOPEN_STREAMS (Obsolescent) - Probably don't want to support this. */
/* TODO: _XOPEN_UNIX - Decide whether we actually support this (probably not),
                       but also whether this header should lie. */
/* TODO: _XOPEN_UUCP - Research what this is. */

/* TODO: _POSIX_ASYNC_IO - Research what exactly this is. */
/* TODO: _POSIX_PRIO_IO - Research what exactly this is. */
/* TODO: _POSIX_SYNC_IO - Research what exactly this is. */
/* TODO: _POSIX_TIMESTAMP_RESOLUTION - Research what exactly this is. */
/* TODO: _POSIX2_SYMLINKS - Research what exactly this is. */

#ifndef NULL
#define __need_NULL
#include <stddef.h>
#endif

#define _CS_PATH 0
#define _CS_POSIX_V7_ILP32_OFF32_CFLAGS 1
#define _CS_POSIX_V7_ILP32_OFF32_LDFLAGS 2
#define _CS_POSIX_V7_ILP32_OFF32_LIBS 3
#define _CS_POSIX_V7_ILP32_OFFBIG_CFLAGS 4
#define _CS_POSIX_V7_ILP32_OFFBIG_LDFLAGS 5
#define _CS_POSIX_V7_ILP32_OFFBIG_LIBS 6
#define _CS_POSIX_V7_LP64_OFF64_CFLAGS 7
#define _CS_POSIX_V7_LP64_OFF64_LDFLAGS 8
#define _CS_POSIX_V7_LP64_OFF64_LIBS 9
#define _CS_POSIX_V7_LPBIG_OFFBIG_CFLAGS 10
#define _CS_POSIX_V7_LPBIG_OFFBIG_LDFLAGS 11
#define _CS_POSIX_V7_LPBIG_OFFBIG_LIBS 12
#define _CS_POSIX_V7_THREADS_CFLAGS 13
#define _CS_POSIX_V7_THREADS_LDFLAGS 14
#define _CS_POSIX_V7_WIDTH_RESTRICTED_ENVS 15
#define _CS_V7_ENV 16
#define _CS_POSIX_V6_ILP32_OFF32_CFLAGS 17 /* obsolescent */
#define _CS_POSIX_V6_ILP32_OFF32_LDFLAGS 18 /* obsolescent */
#define _CS_POSIX_V6_ILP32_OFF32_LIBS 19 /* obsolescent */
#define _CS_POSIX_V6_ILP32_OFFBIG_CFLAGS 20 /* obsolescent */
#define _CS_POSIX_V6_ILP32_OFFBIG_LDFLAGS 21 /* obsolescent */
#define _CS_POSIX_V6_ILP32_OFFBIG_LIBS 22 /* obsolescent */
#define _CS_POSIX_V6_LP64_OFF64_CFLAGS 23 /* obsolescent */
#define _CS_POSIX_V6_LP64_OFF64_LDFLAGS 24 /* obsolescent */
#define _CS_POSIX_V6_LP64_OFF64_LIBS 25 /* obsolescent */
#define _CS_POSIX_V6_LPBIG_OFFBIG_CFLAGS 26 /* obsolescent */
#define _CS_POSIX_V6_LPBIG_OFFBIG_LDFLAGS 27 /* obsolescent */
#define _CS_POSIX_V6_LPBIG_OFFBIG_LIBS 28 /* obsolescent */
#define _CS_POSIX_V6_WIDTH_RESTRICTED_ENVS 29 /* obsolescent */
#define _CS_V6_ENV 30 /* obsolescent */

/* Sortix will not support POSIX advisory locks and doesn't declare:
   F_LOCK
   F_TEST
   F_TLOCK
   F_UNLOCK */

#define _PC_2_SYMLINKS 1
#define _PC_ALLOC_SIZE_MIN 2
#define _PC_ASYNC_IO 3
#define _PC_CHOWN_RESTRICTED 4
#define _PC_FILESIZEBITS 5
#define _PC_LINK_MAX 6
#define _PC_MAX_CANON 7
#define _PC_MAX_INPUT 8
#define _PC_NAME_MAX 9
#define _PC_NO_TRUNC 10
#define _PC_PATH_MAX 11
#define _PC_PIPE_BUF 12
#define _PC_PRIO_IO 13
#define _PC_REC_INCR_XFER_SIZE 14
#define _PC_REC_MAX_XFER_SIZE 15
#define _PC_REC_MIN_XFER_SIZE 16
#define _PC_REC_XFER_ALIGN 17
#define _PC_SYMLINK_MAX 18
#define _PC_SYNC_IO 19
#define _PC_TIMESTAMP_RESOLUTION 20
#define _PC_VDISABLE 21

#define _SC_AIO_LISTIO_MAX 0
#define _SC_AIO_MAX 1
#define _SC_AIO_PRIO_DELTA_MAX 2
#define _SC_ARG_MAX 3
#define _SC_ATEXIT_MAX 4
#define _SC_BC_BASE_MAX 5
#define _SC_BC_DIM_MAX 6
#define _SC_BC_SCALE_MAX 7
#define _SC_BC_STRING_MAX 8
#define _SC_CHILD_MAX 9
#define _SC_CLK_TCK 10
#define _SC_COLL_WEIGHTS_MAX 11
#define _SC_DELAYTIMER_MAX 12
#define _SC_EXPR_NEST_MAX 13
#define _SC_HOST_NAME_MAX 14
#define _SC_IOV_MAX 15
#define _SC_LINE_MAX 16
#define _SC_LOGIN_NAME_MAX 17
#define _SC_NGROUPS_MAX 18
#define _SC_GETGR_R_SIZE_MAX 19
#define _SC_GETPW_R_SIZE_MAX 20
#define _SC_MQ_OPEN_MAX 21
#define _SC_MQ_PRIO_MAX 22
#define _SC_OPEN_MAX 23
#define _SC_ADVISORY_INFO 24
#define _SC_BARRIERS 25
#define _SC_ASYNCHRONOUS_IO 26
#define _SC_CLOCK_SELECTION 27
#define _SC_CPUTIME 28
#define _SC_FSYNC 29
#define _SC_IPV6 30
#define _SC_JOB_CONTROL 31
#define _SC_MAPPED_FILES 32
#define _SC_MEMLOCK 33
#define _SC_MEMLOCK_RANGE 34
#define _SC_MEMORY_PROTECTION 35
#define _SC_MESSAGE_PASSING 36
#define _SC_MONOTONIC_CLOCK 37
#define _SC_PRIORITIZED_IO 38
#define _SC_PRIORITY_SCHEDULING 39
#define _SC_RAW_SOCKETS 40
#define _SC_READER_WRITER_LOCKS 41
#define _SC_REALTIME_SIGNALS 42
#define _SC_REGEXP 43
#define _SC_SAVED_IDS 44
#define _SC_SEMAPHORES 45
#define _SC_SHARED_MEMORY_OBJECTS 46
#define _SC_SHELL 47
#define _SC_SPAWN 48
#define _SC_SPIN_LOCKS 49
#define _SC_SPORADIC_SERVER 50
#define _SC_SS_REPL_MAX 51
#define _SC_SYNCHRONIZED_IO 52
#define _SC_THREAD_ATTR_STACKADDR 53
#define _SC_THREAD_ATTR_STACKSIZE 54
#define _SC_THREAD_CPUTIME 55
#define _SC_THREAD_PRIO_INHERIT 56
#define _SC_THREAD_PRIO_PROTECT 57
#define _SC_THREAD_PRIORITY_SCHEDULING 58
#define _SC_THREAD_PROCESS_SHARED 59
#define _SC_THREAD_ROBUST_PRIO_INHERIT 60
#define _SC_THREAD_ROBUST_PRIO_PROTECT 61
#define _SC_THREAD_SAFE_FUNCTIONS 62
#define _SC_THREAD_SPORADIC_SERVER 63
#define _SC_THREADS 64
#define _SC_TIMEOUTS 65
#define _SC_TIMERS 66
#define _SC_TRACE 67
#define _SC_TRACE_EVENT_FILTER 68
#define _SC_TRACE_EVENT_NAME_MAX 69
#define _SC_TRACE_INHERIT 70
#define _SC_TRACE_LOG 71
#define _SC_TRACE_NAME_MAX 72
#define _SC_TRACE_SYS_MAX 73
#define _SC_TRACE_USER_EVENT_MAX 74
#define _SC_TYPED_MEMORY_OBJECTS 75
#define _SC_VERSION 76
#define _SC_V7_ILP32_OFF32 77
#define _SC_V7_ILP32_OFFBIG 78
#define _SC_V7_LP64_OFF64 79
#define _SC_V7_LPBIG_OFFBIG 80
#define _SC_V6_ILP32_OFF32 81 /* obsolescent */
#define _SC_V6_ILP32_OFFBIG 82 /* obsolescent */
#define _SC_V6_LP64_OFF64 83 /* obsolescent */
#define _SC_V6_LPBIG_OFFBIG 84 /* obsolescent */
#define _SC_2_C_BIND 85
#define _SC_2_C_DEV 86
#define _SC_2_CHAR_TERM 87
#define _SC_2_FORT_DEV 88
#define _SC_2_FORT_RUN 89
#define _SC_2_LOCALEDEF 90
#define _SC_2_PBS 91
#define _SC_2_PBS_ACCOUNTING 92
#define _SC_2_PBS_CHECKPOINT 93
#define _SC_2_PBS_LOCATE 94
#define _SC_2_PBS_MESSAGE 95
#define _SC_2_PBS_TRACK 96
#define _SC_2_SW_DEV 97
#define _SC_2_UPE 98
#define _SC_2_VERSION 99
#define _SC_PAGE_SIZE 100
#define _SC_PAGESIZE 111
#define _SC_THREAD_DESTRUCTOR_ITERATIONS 112
#define _SC_THREAD_KEYS_MAX 113
#define _SC_THREAD_STACK_MIN 114
#define _SC_THREAD_THREADS_MAX 115
#define _SC_RE_DUP_MAX 116
#define _SC_RTSIG_MAX 117
#define _SC_SEM_NSEMS_MAX 118
#define _SC_SEM_VALUE_MAX 119
#define _SC_SIGQUEUE_MAX 120
#define _SC_STREAM_MAX 121
#define _SC_SYMLOOP_MAX 122
#define _SC_TIMER_MAX 123
#define _SC_TTY_NAME_MAX 124
#define _SC_TZNAME_MAX 125
#define _SC_XOPEN_CRYPT 126
#define _SC_XOPEN_ENH_I18N 127
#define _SC_XOPEN_REALTIME 128
#define _SC_XOPEN_REALTIME_THREADS 129
#define _SC_XOPEN_SHM 130
#define _SC_XOPEN_STREAMS 131
#define _SC_XOPEN_UNIX 132
#define _SC_XOPEN_UUCP 133
#define _SC_XOPEN_VERSION 134

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#define _POSIX_VDISABLE '\0'

#ifndef __size_t_defined
#define __size_t_defined
#define __need_size_t
#include <stddef.h>
#endif

#ifndef __ssize_t_defined
#define __ssize_t_defined
typedef __ssize_t ssize_t;
#endif

#ifndef __uid_t_defined
#define __uid_t_defined
typedef __uid_t uid_t;
#endif

#ifndef __gid_t_defined
#define __gid_t_defined
typedef __gid_t gid_t;
#endif

#ifndef __off_t_defined
#define __off_t_defined
typedef __off_t off_t;
#endif

#ifndef __pid_t_defined
#define __pid_t_defined
typedef __pid_t pid_t;
#endif

/* TODO: intptr_t is not declared because <stdint.h> doesn't allow other headers
         to define some, but not all, of the fixed width types. Additionally,
         intptr_t was only added for the sake of sbrk(), but that was removed in
         POSIX 2001. */

/* Somehow programs are required to declare environ themselves according to
   the POSIX specification. */
#if __USE_SORTIX
extern char** environ;
#if defined(__is_sortix_libc)
extern char** __environ_malloced;
extern size_t __environ_used;
extern size_t __environ_length;
#endif
#endif

int access(const char*, int);
unsigned alarm(unsigned);
int chdir(const char*);
int chown(const char*, uid_t, gid_t);
int close(int);
int dup(int);
int dup2(int, int);
void _exit(int)  __attribute__ ((noreturn));
int execl(const char*, const char*, ...);
int execle(const char*, const char*, ...);
int execlp(const char*, const char*, ...);
int execv(const char*, char* const []);
int execve(const char*, char* const [], char* const []);
int execvp(const char*, char* const []);
pid_t fork(void);
long fpathconf(int, int);
int fsync(int);
char* getcwd(char*, size_t);
gid_t getegid(void);
uid_t geteuid(void);
gid_t getgid(void);
/* TODO: int getgroups(int, gid_t []); */
char* getlogin(void);
pid_t getpgid(pid_t);
/* TODO: pid_t getpgrp(void); */
pid_t getpid(void);
pid_t getppid(void);
uid_t getuid(void);
int isatty(int);
int link(const char*, const char*);
/* lockf will not be implemented. */
off_t lseek(int, off_t, int);
long pathconf(const char*, int);
/* TODO: int pause(void); */
int pipe(int [2]);
ssize_t read(int, void*, size_t);
int rmdir(const char*);
int setgid(gid_t);
int setpgid(pid_t, pid_t);
/* TODO: pid_t setsid(void); */
int setuid(uid_t);
unsigned sleep(unsigned);
long sysconf(int);
pid_t tcgetpgrp(int);
int tcsetpgrp(int, pid_t);
char* ttyname(int);
int ttyname_r(int, char*, size_t);
int unlink(const char*);
ssize_t write(int, const void*, size_t);

#if __USE_SORTIX || 199209L <= __USE_POSIX
size_t confstr(int, char*, size_t);
/* For compatibility with POSIX, declare getopt(3) here. */
/* These declarations are repeated in <getopt.h>. */
#ifndef __getopt_unistd_shared_declared
#define __getopt_unistd_shared_declared
extern char* optarg;
extern int opterr;
extern int optind;
extern int optopt;

int getopt(int, char* const*, const char*);
#endif
#endif

#if __USE_XOPEN
/* TODO: char* crypt(const char*, const char*); */
/* TODO: void encrypt(char [64], int); */
/* gethostid will not be implemented */
/* TODO: int nice(int); */
/* setpgrp will not be implemented. */
/* TODO: void swab(const void* __restrict, void* __restrict, ssize_t); */
/* TODO: void sync(void); */
#endif

#if __USE_SORTIX || 420 <= __USE_XOPEN
/* TODO: int setregid(gid_t, gid_t); (XSI option) */
/* TODO: int setreuid(uid_t, uid_t); (XSI option) */
#endif

#if __USE_SORTIX || 199309L <= __USE_POSIX || 420 <= __USE_XOPEN
int getlogin_r(char*, size_t);
#endif

#if __USE_SORTIX || 200112L <= __USE_POSIX || 420 <= __USE_XOPEN
int ftruncate(int, off_t);
ssize_t readlink(const char* __restrict, char* __restrict, size_t);
int symlink(const char*, const char*);
#endif

#if __USE_SORTIX || 200809L <= __USE_POSIX || 420 <= __USE_XOPEN
int fchdir(int);
int fchown(int, uid_t, gid_t);
/* TODO: pid_t getsid(void); */
int lchown(const char*, uid_t, gid_t);
int truncate(const char*, off_t);
#endif

/* TODO: This feature macro seems wrong, verify historically. */
#if __USE_SORTIX || 199309L <= __USE_POSIX || 500 <= __USE_XOPEN
/* TODO: fdatasync (SIO option) */
#endif

/* Functions from POSIX 1995. */
#if __USE_SORTIX || 199506L <= __USE_POSIX
int getlogin_r(char*, size_t);
#endif

#if __USE_SORTIX || 200112L <= __USE_POSIX || 500 <= __USE_XOPEN
int gethostname(char*, size_t);
#endif

#if __USE_SORTIX || 200809L <= __USE_POSIX || 500 <= __USE_XOPEN
ssize_t pread(int, void*, size_t, off_t);
ssize_t pwrite(int, const void*, size_t, off_t);
#endif

/* Functions from POSIX 2001. */
#if __USE_SORTIX || 200112L <= __USE_POSIX
int setegid(gid_t);
int seteuid(uid_t);
#endif

/* Functions from POSIX 2008. */
#if __USE_SORTIX || 200809L <= __USE_POSIX
int faccessat(int, const char*, int, int);
int fchownat(int, const char*, uid_t, gid_t, int);
/* TODO: int fexecve(int, char* const [], char* const []); */
int linkat(int, const char*, int, const char*, int);
ssize_t readlinkat(int, const char* __restrict, char* __restrict, size_t);
int symlinkat(const char*, int, const char*);
int unlinkat(int, const char*, int);
#endif

#if __USE_SORTIX || !(200112L <= __USE_POSIX || 600 <= __USE_XOPEN)
size_t getpagesize(void);
#endif

/* Functions copied from elsewhere. */
#if __USE_SORTIX
int chroot(const char*);
int closefrom(int);
int dup3(int, int, int);
int execvpe(const char*, char* const [], char* const []);
char* get_current_dir_name(void);
int getdomainname(char*, size_t);
int getentropy(void*, size_t);
int pipe2(int [2], int);
int sethostname(const char*, size_t);
typedef unsigned int useconds_t;
int usleep(useconds_t useconds);
#endif

/* Functions that are Sortix extensions. */
#if __USE_SORTIX
int alarmns(const struct timespec* delay, struct timespec* odelay);
int exit_thread(int, int, const struct exit_thread*);
int fchdirat(int, const char*);
int fchroot(int);
int fchrootat(int, const char*);
int memstat(size_t* memused, size_t* memtotal);
int mkpartition(int fd, off_t start, off_t length);
pid_t sfork(int flags);
pid_t tfork(int flags, struct tfork* regs);
int truncateat(int dirfd, const char*, off_t);
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
