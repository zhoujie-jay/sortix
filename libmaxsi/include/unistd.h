/*******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011, 2012.

	This file is part of LibMaxsi.

	LibMaxsi is free software: you can redistribute it and/or modify it under
	the terms of the GNU Lesser General Public License as published by the Free
	Software Foundation, either version 3 of the License, or (at your option)
	any later version.

	LibMaxsi is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
	details.

	You should have received a copy of the GNU Lesser General Public License
	along with LibMaxsi. If not, see <http://www.gnu.org/licenses/>.

	unistd.h
	The <unistd.h> header defines miscellaneous symbolic constants and types,
	and declares miscellaneous functions.

*******************************************************************************/

/* TODO: POSIX-1.2008 compliance is only partial */

#ifndef	_UNISTD_H
#define	_UNISTD_H 1

#include <features.h>
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

/* TODO: _PC_* is missing here. */

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
#if defined(_SORTIX_SOURCE) || defined(_SORTIX_ALWAYS_SBRK)
@include(intn_t.h)
#endif

/* TODO: These are not implemented in libmaxsi/sortix yet. */
#if defined(__SORTIX_SHOW_UNIMPLEMENTED)
unsigned alarm(unsigned);
int chown(const char*, uid_t, gid_t);
size_t confstr(int, char*, size_t);
char* crypt(const char*, const char*);
char* ctermid(char*);
int dup2(int, int);
void encrypt(char [64], int);
int execl(const char*, const char*, ...);
int execle(const char*, const char*, ...);
int execlp(const char*, const char*, ...);
int execvp(const char*, char* const []);
int faccessat(int, const char*, int, int);
int fchdir(int);
int fchown(int, uid_t, gid_t);
int fchownat(int, const char*, uid_t, gid_t, int);
int fdatasync(int);
int fexecve(int, char* const [], char* const []);
long fpathconf(int, int);
int fsync(int);
gid_t getegid(void);
uid_t geteuid(void);
gid_t getgid(void);
int getgroups(int, gid_t []);
long gethostid(void);
int gethostname(char*, size_t);
char* getlogin(void);
int getlogin_r(char*, size_t);
int getopt(int, char* const [], const char*);
pid_t getpgid(pid_t);
pid_t getpgrp(void);
pid_t getsid(pid_t);
uid_t getuid(void);
int lchown(const char*, uid_t, gid_t);
int link(const char*, const char*);
int linkat(int, const char*, int, const char*, int);
int lockf(int, int, off_t);
int nice(int);
long pathconf(const char*, int);
int pause(void);
ssize_t readlink(const char* restrict, char* restrict, size_t);
ssize_t readlinkat(int, const char* restrict, char* restrict, size_t);
int setegid(gid_t);
int seteuid(uid_t);
int setgid(gid_t);
int setpgid(pid_t, pid_t);
int setregid(gid_t, gid_t);
int setreuid(uid_t, uid_t);
pid_t setsid(void);
int setuid(uid_t);
void swab(const void* restrict, void* restrict, ssize_t);
int symlink(const char*, const char*);
int symlinkat(const char*, int, const char*);
void sync(void);
long sysconf(int);
pid_t tcgetpgrp(int);
int tcsetpgrp(int, pid_t);
char* ttyname(int);
int ttyname_r(int, char*, size_t);
int unlinkat(int, const char*, int);

#if __POSIX_OBSOLETE <= 200801
pid_t setpgrp(void);
#endif

extern char* optarg;
extern int opterr, optind, optopt;
#endif

int access(const char*, int);
int chdir(const char*);
int close(int);
int dup(int);
void _exit(int);
int execv(const char*, char* const []);
int execve(const char*, char* const [], char* const []);
pid_t fork(void);
int ftruncate(int, off_t);
char* getcwd(char*, size_t);
pid_t getpid(void);
pid_t getppid(void);
int isatty(int);
off_t lseek(int, off_t, int);
int pipe(int [2]);
ssize_t pread(int, void*, size_t, off_t);
ssize_t pwrite(int, const void*, size_t, off_t);
ssize_t read(int, void*, size_t);
int rmdir(const char*);
unsigned sleep(unsigned);
int truncate(const char*, off_t);
#if __POSIX_OBSOLETE <= 200112 || defined(_SORTIX_SOURCE)
int usleep(useconds_t useconds);
#endif
int unlink(const char*);
ssize_t write(int, const void*, size_t);

#if defined(_SORTIX_SOURCE)
size_t getpagesize(void);
int memstat(size_t* memused, size_t* memtotal);
size_t preadall(int fd, void* buf, size_t count, off_t off);
size_t preadleast(int fd, void* buf, size_t least, size_t max, off_t off);
size_t pwriteall(int fd, const void* buf, size_t count, off_t off);
size_t pwriteleast(int fd, const void* buf, size_t least, size_t max, off_t off);
size_t readall(int fd, void* buf, size_t count);
size_t readleast(int fd, void* buf, size_t least, size_t max);
pid_t sfork(int flags);
int uptime(uintmax_t* usecssinceboot);
size_t writeall(int fd, const void* buf, size_t count);
size_t writeleast(int fd, const void* buf, size_t least, size_t max);
#endif
#if defined(_SORTIX_SOURCE) || defined(_SORTIX_ALWAYS_SBRK)
void* sbrk(intptr_t increment);
#endif

__END_DECLS

#endif

