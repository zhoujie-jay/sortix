/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2014.

    This file is part of Sortix.

    Sortix is free software: you can redistribute it and/or modify it under the
    terms of the GNU General Public License as published by the Free Software
    Foundation, either version 3 of the License, or (at your option) any later
    version.

    Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
    details.

    You should have received a copy of the GNU General Public License along with
    Sortix. If not, see <http://www.gnu.org/licenses/>.

    sortix/kernel/syscall.h
    Handles system calls from user-space.

*******************************************************************************/

#ifndef INCLUDE_SORTIX_KERNEL_SYSCALL_H
#define INCLUDE_SORTIX_KERNEL_SYSCALL_H

#include <sys/socket.h>
#include <sys/types.h>

#include <stddef.h>
#include <stdint.h>

#include <sortix/dirent.h>
#include <sortix/exit.h>
#include <sortix/fork.h>
#include <sortix/itimerspec.h>
#include <sortix/poll.h>
#include <sortix/resource.h>
#include <sortix/sigaction.h>
#include <sortix/sigevent.h>
#include <sortix/sigset.h>
#include <sortix/stack.h>
#include <sortix/stat.h>
#include <sortix/statvfs.h>
#include <sortix/syscall.h>
#include <sortix/termios.h>
#include <sortix/timespec.h>
#include <sortix/tmns.h>

namespace Sortix {

struct mmap_request;

int sys_accept4(int, void*, size_t*, int);
int sys_alarmns(const struct timespec*, struct timespec*);
int sys_bad_syscall(void);
int sys_bind(int, const void*, size_t);
int sys_clock_gettimeres(clockid_t, struct timespec*, struct timespec*);
int sys_clock_nanosleep(clockid_t, int, const struct timespec*, struct timespec*);
int sys_clock_settimeres(clockid_t, const struct timespec*, const struct timespec*);
int sys_close(int);
int sys_connect(int, const void*, size_t);
int sys_dispmsg_issue(void*, size_t);
int sys_dup(int);
int sys_dup2(int, int);
int sys_dup3(int, int, int);
int sys_execve(const char*, char* const [], char* const []);
int sys_exit_thread(int, int, const struct exit_thread*);
int sys_faccessat(int, const char*, int, int);
int sys_fchdir(int);
int sys_fchdirat(int, const char*);
int sys_fchmod(int, mode_t);
int sys_fchmodat(int, const char*, mode_t, int);
int sys_fchown(int, uid_t, gid_t);
int sys_fchownat(int, const char*, uid_t, gid_t, int);
int sys_fchroot(int);
int sys_fchrootat(int, const char*);
int sys_fcntl(int, int, uintptr_t);
int sys_fsm_fsbind(int, int, int);
int sys_fstat(int, struct stat*);
int sys_fstatat(int, const char*, struct stat*, int);
int sys_fstatvfs(int, struct statvfs*);
int sys_fstatvfsat(int, const char*, struct statvfs*, int);
int sys_fsync(int);
int sys_ftruncate(int, off_t);
int sys_futimens(int, const struct timespec [2]);
gid_t sys_getegid(void);
int sys_getentropy(void*, size_t);
uid_t sys_geteuid(void);
gid_t sys_getgid(void);
int sys_gethostname(char*, size_t);
size_t sys_getpagesize(void);
int sys_getpeername(int, struct sockaddr*, socklen_t*);
pid_t sys_getpgid(pid_t);
pid_t sys_getpid(void);
pid_t sys_getppid(void);
int sys_getpriority(int, id_t);
int sys_getsockname(int, struct sockaddr*, socklen_t*);
int sys_getsockopt(int, int, int, void*, size_t*);
int sys_gettermmode(int, unsigned*);
uid_t sys_getuid(void);
mode_t sys_getumask(void);
int sys_ioctl(int, int, void*);
int sys_isatty(int);
ssize_t sys_kernelinfo(const char*, char*, size_t);
int sys_kill(pid_t, int);
int sys_linkat(int, const char*, int, const char*, int);
off_t sys_lseek(int, off_t, int);
int sys_listen(int, int);
int sys_memstat(size_t*, size_t*);
int sys_mkdirat(int, const char*, mode_t);
int sys_mkpartition(int, off_t, off_t, int);
void* sys_mmap_wrapper(struct mmap_request*);
int sys_mprotect(const void*, size_t, int);
int sys_munmap(void*, size_t);
int sys_openat(int, const char*, int, mode_t);
int sys_pipe2(int [2], int);
int sys_ppoll(struct pollfd*, nfds_t, const struct timespec*, const sigset_t*);
ssize_t sys_pread(int, void*, size_t, off_t);
ssize_t sys_preadv(int, const struct iovec*, int, off_t);
int sys_prlimit(pid_t, int, const struct rlimit*, struct rlimit*);
ssize_t sys_pwrite(int, const void*, size_t, off_t);
ssize_t sys_pwritev(int, const struct iovec*, int, off_t);
int sys_raise(int);
uint64_t sys_rdmsr(uint32_t);
ssize_t sys_read(int, void*, size_t);
ssize_t sys_readdirents(int, struct kernel_dirent*, size_t);
ssize_t sys_readlinkat(int, const char*, char*, size_t);
ssize_t sys_readv(int, const struct iovec*, int);
ssize_t sys_recv(int, void*, size_t, int);
ssize_t sys_recvmsg(int, struct msghdr*, int);
int sys_renameat(int, const char*, int, const char*);
int sys_sched_yield(void);
ssize_t sys_send(int, const void*, size_t, int);
ssize_t sys_sendmsg(int, const struct msghdr*, int);
int sys_setegid(gid_t);
int sys_seteuid(uid_t);
int sys_setgid(gid_t);
int sys_sethostname(const char*, size_t);
int sys_setpgid(pid_t, pid_t);
int sys_setpriority(int, id_t, int);
int sys_setsockopt(int, int, int, const void*, size_t);
int sys_settermmode(int, unsigned);
int sys_setuid(uid_t);
int sys_shutdown(int, int);
int sys_sigaction(int, const struct sigaction*, struct sigaction*);
int sys_sigaltstack(const stack_t*, stack_t*);
int sys_sigpending(sigset_t*);
int sys_sigprocmask(int, const sigset_t*, sigset_t*);
int sys_sigsuspend(const sigset_t*);
int sys_symlinkat(const char*, int, const char*);
ssize_t sys_tcgetblob(int, const char*, void*, size_t);
int sys_tcgetpgrp(int);
int sys_tcgetwincurpos(int, struct wincurpos*);
int sys_tcgetwinsize(int, struct winsize*);
ssize_t sys_tcsetblob(int, const char*, const void*, size_t);
int sys_tcsetpgrp(int, pid_t);
pid_t sys_tfork(int, struct tfork*);
int sys_timens(struct tmns*);
int sys_timer_create(clockid_t clockid, struct sigevent*, timer_t*);
int sys_timer_delete(timer_t);
int sys_timer_getoverrun(timer_t);
int sys_timer_gettime(timer_t, struct itimerspec*);
int sys_timer_settime(timer_t, int, const struct itimerspec*, struct itimerspec*);
int sys_truncateat(int, const char*, off_t);
mode_t sys_umask(mode_t);
int sys_unlinkat(int, const char*, int);
int sys_utimensat(int, const char*, const struct timespec [2], int);
pid_t sys_waitpid(pid_t, int*, int);
ssize_t sys_write(int, const void*, size_t);
ssize_t sys_writev(int, const struct iovec*, int);
uint64_t sys_wrmsr(uint32_t, uint64_t);

} // namespace Sortix

#endif
