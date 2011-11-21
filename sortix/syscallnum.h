/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011.

	This file is part of Sortix.

	Sortix is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License as published by the Free Software
	Foundation, either version 3 of the License, or (at your option) any later
	version.

	Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
	details.

	You should have received a copy of the GNU General Public License along
	with Sortix. If not, see <http://www.gnu.org/licenses/>.

	syscallnum.h
	Stores numerical constants for each available system call on this kernel.

******************************************************************************/

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
#define SYSCALL_RECEIVE_KEYSTROKE 8
#define SYSCALL_SET_FREQUENCY 9
#define SYSCALL_EXEC 10
#define SYSCALL_PRINT_PATH_FILES 11
#define SYSCALL_FORK 12
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
#define SYSCALL_MAX_NUM 28 /* index of highest constant + 1 */

#endif

