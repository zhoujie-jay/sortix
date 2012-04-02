/*******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2012.

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

	unistd.h
	Standard symbolic constants and types.

*******************************************************************************/

#ifndef SORTIX_UNISTD_H
#define SORTIX_UNISTD_H

#include <features.h>

__BEGIN_DECLS

#define R_OK 4 /* Test for read permission. */
#define W_OK 2 /* Test for write permission. */
#define X_OK 1 /* Test for execute permission. */
#define F_OK 0 /* Test for existence. */

#define RFCLONE (1<<0) /* Creates child, otherwise affect current process. */
#define RFPID (1<<1) /* Allocates new PID. */
#define RFFDG (1<<2) /* Fork file descriptor table. */
#define RFMEM (1<<3) /* Shares address space. */
#define RFFMEM (1<<4) /* Forks address space. */
#define RFCWD (1<<5) /* Forks current directory pointer. */
#define RFNS (1<<6) /* Forks namespace. */

#define RFPROC (RFCLONE | RFPID | RFFMEM | RFCWD) /* Create new process. */
#define RFFORK (RFPROC | RFFDG) /* Traditional fork(2) behavior. */

__END_DECLS

#endif

