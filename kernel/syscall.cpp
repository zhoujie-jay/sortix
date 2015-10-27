/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2014, 2015.

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

    syscall.cpp
    Handles system calls from user-space.

*******************************************************************************/

#include <errno.h>
#include <stddef.h>

#include <sortix/syscall.h>

#include <sortix/kernel/log.h>
#include <sortix/kernel/syscall.h>

namespace Sortix {

extern "C" {
void* syscall_list[SYSCALL_MAX_NUM + 1] =
{
	[SYSCALL_BAD_SYSCALL] = (void*) sys_bad_syscall,
	[SYSCALL_EXIT] = (void*) sys_bad_syscall,
	[SYSCALL_SLEEP] = (void*) sys_bad_syscall,
	[SYSCALL_USLEEP] = (void*) sys_bad_syscall,
	[SYSCALL_PRINT_STRING] = (void*) sys_bad_syscall,
	[SYSCALL_CREATE_FRAME] = (void*) sys_bad_syscall,
	[SYSCALL_CHANGE_FRAME] = (void*) sys_bad_syscall,
	[SYSCALL_DELETE_FRAME] = (void*) sys_bad_syscall,
	[SYSCALL_RECEIVE_KEYSTROKE] = (void*) sys_bad_syscall,
	[SYSCALL_SET_FREQUENCY] = (void*) sys_bad_syscall,
	[SYSCALL_EXECVE] = (void*) sys_execve,
	[SYSCALL_PRINT_PATH_FILES] = (void*) sys_bad_syscall,
	[SYSCALL_FORK] = (void*) sys_bad_syscall,
	[SYSCALL_GETPID] = (void*) sys_getpid,
	[SYSCALL_GETPPID] = (void*) sys_getppid,
	[SYSCALL_GET_FILEINFO] = (void*) sys_bad_syscall,
	[SYSCALL_GET_NUM_FILES] = (void*) sys_bad_syscall,
	[SYSCALL_WAITPID] = (void*) sys_waitpid,
	[SYSCALL_READ] = (void*) sys_read,
	[SYSCALL_WRITE] = (void*) sys_write,
	[SYSCALL_PIPE] = (void*) sys_bad_syscall,
	[SYSCALL_CLOSE] = (void*) sys_close,
	[SYSCALL_DUP] = (void*) sys_dup,
	[SYSCALL_OPEN] = (void*) sys_bad_syscall,
	[SYSCALL_READDIRENTS] = (void*) sys_readdirents,
	[SYSCALL_CHDIR] = (void*) sys_bad_syscall,
	[SYSCALL_GETCWD] = (void*) sys_bad_syscall,
	[SYSCALL_UNLINK] = (void*) sys_bad_syscall,
	[SYSCALL_REGISTER_ERRNO] = (void*) sys_bad_syscall,
	[SYSCALL_REGISTER_SIGNAL_HANDLER] = (void*) sys_bad_syscall,
	[SYSCALL_SIGRETURN] = (void*) sys_bad_syscall,
	[SYSCALL_KILL] = (void*) sys_kill,
	[SYSCALL_MEMSTAT] = (void*) sys_memstat,
	[SYSCALL_ISATTY] = (void*) sys_isatty,
	[SYSCALL_UPTIME] = (void*) sys_bad_syscall,
	[SYSCALL_SBRK] = (void*) sys_bad_syscall,
	[SYSCALL_LSEEK] = (void*) sys_lseek,
	[SYSCALL_GETPAGESIZE] = (void*) sys_getpagesize,
	[SYSCALL_MKDIR] = (void*) sys_bad_syscall,
	[SYSCALL_RMDIR] = (void*) sys_bad_syscall,
	[SYSCALL_TRUNCATE] = (void*) sys_bad_syscall,
	[SYSCALL_FTRUNCATE] = (void*) sys_ftruncate,
	[SYSCALL_SETTERMMODE] = (void*) sys_settermmode,
	[SYSCALL_GETTERMMODE] = (void*) sys_gettermmode,
	[SYSCALL_STAT] = (void*) sys_bad_syscall,
	[SYSCALL_FSTAT] = (void*) sys_fstat,
	[SYSCALL_FCNTL] = (void*) sys_fcntl,
	[SYSCALL_ACCESS] = (void*) sys_bad_syscall,
	[SYSCALL_KERNELINFO] = (void*) sys_kernelinfo,
	[SYSCALL_PREAD] = (void*) sys_pread,
	[SYSCALL_PWRITE] = (void*) sys_pwrite,
	[SYSCALL_TFORK] = (void*) sys_tfork,
	[SYSCALL_TCGETWINSIZE] = (void*) sys_tcgetwinsize,
	[SYSCALL_RAISE] = (void*) sys_raise,
	[SYSCALL_OPENAT] = (void*) sys_openat,
	[SYSCALL_DISPMSG_ISSUE] = (void*) sys_dispmsg_issue,
	[SYSCALL_FSTATAT] = (void*) sys_fstatat,
	[SYSCALL_CHMOD] = (void*) sys_bad_syscall,
	[SYSCALL_CHOWN] = (void*) sys_bad_syscall,
	[SYSCALL_LINK] = (void*) sys_bad_syscall,
	[SYSCALL_DUP2] = (void*) sys_dup2,
	[SYSCALL_UNLINKAT] = (void*) sys_unlinkat,
	[SYSCALL_FACCESSAT] = (void*) sys_faccessat,
	[SYSCALL_MKDIRAT] = (void*) sys_mkdirat,
	[SYSCALL_FCHDIR] = (void*) sys_fchdir,
	[SYSCALL_TRUNCATEAT] = (void*) sys_truncateat,
	[SYSCALL_FCHOWNAT] = (void*) sys_fchownat,
	[SYSCALL_FCHOWN] = (void*) sys_fchown,
	[SYSCALL_FCHMOD] = (void*) sys_fchmod,
	[SYSCALL_FCHMODAT] = (void*) sys_fchmodat,
	[SYSCALL_LINKAT] = (void*) sys_linkat,
	[SYSCALL_FSM_FSBIND] = (void*) sys_fsm_fsbind,
	[SYSCALL_PPOLL] = (void*) sys_ppoll,
	[SYSCALL_RENAMEAT] = (void*) sys_renameat,
	[SYSCALL_READLINKAT] = (void*) sys_readlinkat,
	[SYSCALL_FSYNC] = (void*) sys_fsync,
	[SYSCALL_GETUID] = (void*) sys_getuid,
	[SYSCALL_GETGID] = (void*) sys_getgid,
	[SYSCALL_SETUID] = (void*) sys_setuid,
	[SYSCALL_SETGID] = (void*) sys_setgid,
	[SYSCALL_GETEUID] = (void*) sys_geteuid,
	[SYSCALL_GETEGID] = (void*) sys_getegid,
	[SYSCALL_SETEUID] = (void*) sys_seteuid,
	[SYSCALL_SETEGID] = (void*) sys_setegid,
	[SYSCALL_IOCTL] = (void*) sys_ioctl,
	[SYSCALL_UTIMENSAT] = (void*) sys_utimensat,
	[SYSCALL_FUTIMENS] = (void*) sys_futimens,
	[SYSCALL_RECV] = (void*) sys_recv,
	[SYSCALL_SEND] = (void*) sys_send,
	[SYSCALL_ACCEPT4] = (void*) sys_accept4,
	[SYSCALL_BIND] = (void*) sys_bind,
	[SYSCALL_CONNECT] = (void*) sys_connect,
	[SYSCALL_LISTEN] = (void*) sys_listen,
	[SYSCALL_READV] = (void*) sys_readv,
	[SYSCALL_WRITEV] = (void*) sys_writev,
	[SYSCALL_PREADV] = (void*) sys_preadv,
	[SYSCALL_PWRITEV] = (void*) sys_pwritev,
	[SYSCALL_TIMER_CREATE] = (void*) sys_timer_create,
	[SYSCALL_TIMER_DELETE] = (void*) sys_timer_delete,
	[SYSCALL_TIMER_GETOVERRUN] = (void*) sys_timer_getoverrun,
	[SYSCALL_TIMER_GETTIME] = (void*) sys_timer_gettime,
	[SYSCALL_TIMER_SETTIME] = (void*) sys_timer_settime,
	[SYSCALL_ALARMNS] = (void*) sys_alarmns,
	[SYSCALL_CLOCK_GETTIMERES] = (void*) sys_clock_gettimeres,
	[SYSCALL_CLOCK_SETTIMERES] = (void*) sys_clock_settimeres,
	[SYSCALL_CLOCK_NANOSLEEP] = (void*) sys_clock_nanosleep,
	[SYSCALL_TIMENS] = (void*) sys_timens,
	[SYSCALL_UMASK] = (void*) sys_umask,
	[SYSCALL_FCHDIRAT] = (void*) sys_fchdirat,
	[SYSCALL_FCHROOT] = (void*) sys_fchroot,
	[SYSCALL_FCHROOTAT] = (void*) sys_fchrootat,
	[SYSCALL_MKPARTITION] = (void*) sys_mkpartition,
	[SYSCALL_GETPGID] = (void*) sys_getpgid,
	[SYSCALL_SETPGID] = (void*) sys_setpgid,
	[SYSCALL_TCGETPGRP] = (void*) sys_tcgetpgrp,
	[SYSCALL_TCSETPGRP] = (void*) sys_tcsetpgrp,
	[SYSCALL_MMAP_WRAPPER] = (void*) sys_mmap_wrapper,
	[SYSCALL_MPROTECT] = (void*) sys_mprotect,
	[SYSCALL_MUNMAP] = (void*) sys_munmap,
	[SYSCALL_GETPRIORITY] = (void*) sys_getpriority,
	[SYSCALL_SETPRIORITY] = (void*) sys_setpriority,
	[SYSCALL_PRLIMIT] = (void*) sys_prlimit,
	[SYSCALL_DUP3] = (void*) sys_dup3,
	[SYSCALL_SYMLINKAT] = (void*) sys_symlinkat,
	[SYSCALL_TCGETWINCURPOS] = (void*) sys_tcgetwincurpos,
	[SYSCALL_PIPE2] = (void*) sys_pipe2,
	[SYSCALL_GETUMASK] = (void*) sys_getumask,
	[SYSCALL_FSTATVFS] = (void*) sys_fstatvfs,
	[SYSCALL_FSTATVFSAT] = (void*) sys_fstatvfsat,
	[SYSCALL_RDMSR] = (void*) sys_rdmsr,
	[SYSCALL_WRMSR] = (void*) sys_wrmsr,
	[SYSCALL_SCHED_YIELD] = (void*) sys_sched_yield,
	[SYSCALL_EXIT_THREAD] = (void*) sys_exit_thread,
	[SYSCALL_SIGACTION] = (void*) sys_sigaction,
	[SYSCALL_SIGALTSTACK] = (void*) sys_sigaltstack,
	[SYSCALL_SIGPENDING] = (void*) sys_sigpending,
	[SYSCALL_SIGPROCMASK] = (void*) sys_sigprocmask,
	[SYSCALL_SIGSUSPEND] = (void*) sys_sigsuspend,
	[SYSCALL_SENDMSG] = (void*) sys_sendmsg,
	[SYSCALL_RECVMSG] = (void*) sys_recvmsg,
	[SYSCALL_GETSOCKOPT] = (void*) sys_getsockopt,
	[SYSCALL_SETSOCKOPT] = (void*) sys_setsockopt,
	[SYSCALL_TCGETBLOB] = (void*) sys_tcgetblob,
	[SYSCALL_TCSETBLOB] = (void*) sys_tcsetblob,
	[SYSCALL_GETPEERNAME] = (void*) sys_getpeername,
	[SYSCALL_GETSOCKNAME] = (void*) sys_getsockname,
	[SYSCALL_SHUTDOWN] = (void*) sys_shutdown,
	[SYSCALL_GETENTROPY] = (void*) sys_getentropy,
	[SYSCALL_GETHOSTNAME] = (void*) sys_gethostname,
	[SYSCALL_SETHOSTNAME] = (void*) sys_sethostname,
	[SYSCALL_UNMOUNTAT] = (void*) sys_unmountat,
	[SYSCALL_FSM_MOUNTAT] = (void*) sys_fsm_mountat,
	[SYSCALL_CLOSEFROM] = (void*) sys_closefrom,
	[SYSCALL_RESERVED1] = (void*) sys_bad_syscall,
	[SYSCALL_PSCTL] = (void*) sys_psctl,
	[SYSCALL_RESERVED2] = (void*) sys_bad_syscall,
	[SYSCALL_RESERVED3] = (void*) sys_bad_syscall,
	[SYSCALL_RESERVED4] = (void*) sys_bad_syscall,
	[SYSCALL_RESERVED5] = (void*) sys_bad_syscall,
	[SYSCALL_RESERVED6] = (void*) sys_bad_syscall,
	[SYSCALL_RESERVED7] = (void*) sys_bad_syscall,
	[SYSCALL_RESERVED8] = (void*) sys_bad_syscall,
	[SYSCALL_SCRAM] = (void*) sys_scram,
	[SYSCALL_MAX_NUM] = (void*) sys_bad_syscall,
};
} /* extern "C" */

int sys_bad_syscall(void)
{
	// TODO: Send signal, set errno, or crash/abort process?
	Log::PrintF("I am the bad system call!\n");
	return errno = EINVAL, -1;
}

} // namespace Sortix
