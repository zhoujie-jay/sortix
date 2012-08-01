/*******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011, 2012.

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
	syscallnum.h
	Stores numerical constants for each available system call on this kernel.

*******************************************************************************/

#ifndef SORTIX_SYSCALLNUM_H
#define SORTIX_SYSCALLNUM_H

#define SYSCALL_BAD_SYSCALL 0
#define SYSCALL_EXIT 1
#define SYSCALL_SLEEP 2
#define SYSCALL_USLEEP 3
#define SYSCALL_PRINT_STRING 4
#define SYSCALL_CREATE_FRAME 5
#define SYSCALL_CHANGE_FRAME 6
#define SYSCALL_DELETE_FRAME 7
#define SYSCALL_SET_FREQUENCY 9
#define SYSCALL_EXEC 10
#define SYSCALL_PRINT_PATH_FILES 11
#define SYSCALL_GETPID 13
#define SYSCALL_GETPPID 14
#define SYSCALL_GET_FILEINFO 15
#define SYSCALL_GET_NUM_FILES 16
#define SYSCALL_WAIT 17
#define SYSCALL_READ 18
#define SYSCALL_WRITE 19
#define SYSCALL_PIPE 20
#define SYSCALL_CLOSE 21
#define SYSCALL_DUP 22
#define SYSCALL_OPEN 23
#define SYSCALL_READDIRENTS 24
#define SYSCALL_CHDIR 25
#define SYSCALL_GETCWD 26
#define SYSCALL_UNLINK 27
#define SYSCALL_REGISTER_ERRNO 28
#define SYSCALL_REGISTER_SIGNAL_HANDLER 29
#define SYSCALL_KILL 31
#define SYSCALL_MEMSTAT 32
#define SYSCALL_ISATTY 33
#define SYSCALL_UPTIME 34
#define SYSCALL_SBRK 35
#define SYSCALL_SEEK 36
#define SYSCALL_GET_PAGE_SIZE 37
#define SYSCALL_MKDIR 38
#define SYSCALL_RMDIR 39
#define SYSCALL_TRUNCATE 40
#define SYSCALL_FTRUNCATE 41
#define SYSCALL_SETTERMMODE 42
#define SYSCALL_GETTERMMODE 43
#define SYSCALL_STAT 44
#define SYSCALL_FSTAT 45
#define SYSCALL_FCNTL 46
#define SYSCALL_ACCESS 47
#define SYSCALL_KERNELINFO 48
#define SYSCALL_PREAD 49
#define SYSCALL_PWRITE 50
#define SYSCALL_SFORKR 51
#define SYSCALL_TCGETWINSIZE 52
#define SYSCALL_RAISE 53
#define SYSCALL_MAX_NUM 54 /* index of highest constant + 1 */

#endif

