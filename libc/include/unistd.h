/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

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

#include <features.h>
#include <__/stdint.h>
#if defined(_SORTIX_SOURCE)
#include <stdarg.h>
#include <stdint.h>
#include <sortix/fork.h>
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

/* TODO: _CS_* is missing here. */

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

/* TODO: _SC_* is missing here. */

#define STDIN_FILENO (0)
#define STDOUT_FILENO (1)
#define STDERR_FILENO (2)

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
#if defined(__SORTIX_SHOW_UNIMPLEMENTED)
unsigned alarm(unsigned);
size_t confstr(int, char*, size_t);
char* crypt(const char*, const char*);
char* ctermid(char*);
void encrypt(char [64], int);
int fdatasync(int);
int fexecve(int, char* const [], char* const []);
long fpathconf(int, int);
int getgroups(int, gid_t []);
long gethostid(void);
char* getlogin(void);
int getlogin_r(char*, size_t);
int getopt(int, char* const [], const char*);
pid_t getpgid(pid_t);
pid_t getpgrp(void);
pid_t getsid(pid_t);
int lockf(int, int, off_t);
int nice(int);
int pause(void);
int setpgid(pid_t, pid_t);
int setregid(gid_t, gid_t);
int setreuid(uid_t, uid_t);
pid_t setsid(void);
void swab(const void* __restrict, void* __restrict, ssize_t);
int symlink(const char*, const char*);
int symlinkat(const char*, int, const char*);
void sync(void);
long sysconf(int);
pid_t tcgetpgrp(int);
int tcsetpgrp(int, pid_t);
int ttyname_r(int, char*, size_t);

#if __POSIX_OBSOLETE <= 200801
pid_t setpgrp(void);
#endif

extern char* optarg;
extern int opterr, optind, optopt;
#endif

int access(const char*, int);
int chdir(const char*);
int chown(const char*, uid_t, gid_t);
int close(int);
int dup2(int, int);
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
int fchown(int, uid_t, gid_t);
int fchownat(int, const char*, uid_t, gid_t, int);
int fsync(int);
int ftruncate(int, off_t);
char* getcwd(char*, size_t);
char* get_current_dir_name(void);
gid_t getegid(void);
uid_t geteuid(void);
int gethostname(char*, size_t);
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
int setuid(uid_t);
unsigned sleep(unsigned);
int truncate(const char*, off_t);
int truncateat(int dirfd, const char*, off_t);
char* ttyname(int);
int usleep(useconds_t useconds);
int unlinkat(int, const char*, int);
int unlink(const char*);
ssize_t write(int, const void*, size_t);

#if defined(_SORTIX_SOURCE)
int execvpe(const char*, char* const [], char* const []);
int getdtablesize(void);
size_t getpagesize(void);
int memstat(size_t* memused, size_t* memtotal);
size_t preadall(int fd, void* buf, size_t count, off_t off);
size_t preadleast(int fd, void* buf, size_t least, size_t max, off_t off);
size_t pwriteall(int fd, const void* buf, size_t count, off_t off);
size_t pwriteleast(int fd, const void* buf, size_t least, size_t max, off_t off);
size_t readall(int fd, void* buf, size_t count);
size_t readleast(int fd, void* buf, size_t least, size_t max);
pid_t sfork(int flags);
pid_t tfork(int flags, tforkregs_t* regs);
int uptime(__uintmax_t* usecssinceboot);
size_t writeall(int fd, const void* buf, size_t count);
size_t writeleast(int fd, const void* buf, size_t least, size_t max);
#endif
#if defined(_SORTIX_SOURCE) || defined(_SORTIX_ALWAYS_SBRK)
void* sbrk(__intptr_t increment);
#endif

__END_DECLS

#endif
