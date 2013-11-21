/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013.

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
    The <unistd.h> header defines miscellaneous symbolic constants and types,
    and declares miscellaneous functions.

*******************************************************************************/

/* TODO: POSIX-1.2008 compliance is only partial */

#ifndef _UNISTD_H
#define _UNISTD_H 1

#include <sys/cdefs.h>

#include <sys/__/types.h>
#include <__/stdint.h>

#if defined(_SORTIX_SOURCE)
#include <stdarg.h>
#include <stdint.h>
#include <sortix/fork.h>
__BEGIN_DECLS
@include(time_t.h)
__END_DECLS
#include <sortix/timespec.h>
#include <ioleast.h>
#endif
#include <sortix/seek.h>
#include <sortix/unistd.h>

#define _SORTIX_ALWAYS_SBRK

__BEGIN_DECLS

/* Currently just say we support the newest POSIX. */
/* TODO: Support the newest POSIX. */
#define _POSIX_VERSION 200809L
#define _POSIX2_VERSION	200809L

/* Currently just say we support the newest X/Open */
/* TODO: Support the newest X/Open */
#define _XOPEN_VERSION 700

/* TODO: _POSIX_*, _POSIX2_*, _XOPEN_* is missing here. */

/* TODO: _POSIX_*, _POSIX2_* is missing here. */

@include(NULL.h)

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

/* TODO: F_* is missing here. */

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

/* TODO: _POSIX_VDISABLE is missing here. */

@include(size_t.h)
@include(ssize_t.h)
@include(uid_t.h)
@include(gid_t.h)
@include(off_t.h)
@include(pid_t.h)
@include(useconds_t.h)

#if defined(_WANT_ENVIRON)
extern char** environ;
#endif

/* TODO: These are not implemented in sortix libc yet. */
#if 0
char* crypt(const char*, const char*);
char* ctermid(char*);
void encrypt(char [64], int);
int fdatasync(int);
int fexecve(int, char* const [], char* const []);
long fpathconf(int, int);
int getgroups(int, gid_t []);
long gethostid(void);

pid_t getpgrp(void);
pid_t getsid(pid_t);
int lockf(int, int, off_t);
int nice(int);
int pause(void);
int setregid(gid_t, gid_t);
int setreuid(uid_t, uid_t);
pid_t setsid(void);
void swab(const void* __restrict, void* __restrict, ssize_t);
void sync(void);
int ttyname_r(int, char*, size_t);
#endif

int access(const char*, int);
unsigned alarm(unsigned);
int chdir(const char*);
int chown(const char*, uid_t, gid_t);
int chroot(const char*);
int close(int);
size_t confstr(int, char*, size_t);
int dup2(int, int);
int dup3(int, int, int);
int dup(int);
void _exit(int)  __attribute__ ((noreturn));
int execl(const char*, const char*, ...);
int execle(const char*, const char*, ...);
int execlp(const char*, const char*, ...);
int execv(const char*, char* const []);
int execve(const char*, char* const [], char* const []);
int execvp(const char*, char* const []);
pid_t fork(void);
int faccessat(int, const char*, int, int);
int fchdir(int);
int fchdirat(int, const char*);
int fchown(int, uid_t, gid_t);
int fchownat(int, const char*, uid_t, gid_t, int);
int fchroot(int);
int fchrootat(int, const char*);
int fsync(int);
int ftruncate(int, off_t);
char* getcwd(char*, size_t);
char* get_current_dir_name(void);
gid_t getegid(void);
uid_t geteuid(void);
int gethostname(char*, size_t);
char* getlogin(void);
int getlogin_r(char*, size_t);
pid_t getpgid(pid_t);
pid_t getpid(void);
pid_t getppid(void);
uid_t getuid(void);
gid_t getgid(void);
int isatty(int);
int lchown(const char*, uid_t, gid_t);
int link(const char*, const char*);
int linkat(int, const char*, int, const char*, int);
off_t lseek(int, off_t, int);
long pathconf(const char*, int);
int pipe(int [2]);
ssize_t pread(int, void*, size_t, off_t);
ssize_t pwrite(int, const void*, size_t, off_t);
ssize_t readlink(const char* __restrict, char* __restrict, size_t);
ssize_t readlinkat(int, const char* __restrict, char* __restrict, size_t);
ssize_t read(int, void*, size_t);
int rmdir(const char*);
int setegid(gid_t);
int seteuid(uid_t);
int setgid(gid_t);
int setpgid(pid_t, pid_t);
int setuid(uid_t);
unsigned sleep(unsigned);
int symlink(const char*, const char*);
int symlinkat(const char*, int, const char*);
long sysconf(int);
pid_t tcgetpgrp(int);
int tcsetpgrp(int, pid_t);
int truncate(const char*, off_t);
int truncateat(int dirfd, const char*, off_t);
char* ttyname(int);
int usleep(useconds_t useconds);
int unlinkat(int, const char*, int);
int unlink(const char*);
ssize_t write(int, const void*, size_t);

#if defined(_SORTIX_SOURCE)
int alarmns(const struct timespec* delay, struct timespec* odelay);
int execvpe(const char*, char* const [], char* const []);
size_t getpagesize(void);
int memstat(size_t* memused, size_t* memtotal);
int mkpartition(int fd, off_t start, off_t length);
pid_t sfork(int flags);
pid_t tfork(int flags, tforkregs_t* regs);
size_t writeall(int fd, const void* buf, size_t count);
size_t writeleast(int fd, const void* buf, size_t least, size_t max);
#endif
#if defined(_SORTIX_SOURCE) || defined(_SORTIX_ALWAYS_SBRK)
void* sbrk(__intptr_t increment);
#endif

/* For compatibility with POSIX, declare getopt(3) here. */
#if !defined(_SORTIX_SOURCE)
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

__END_DECLS

#endif
