/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012.

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

    sortix/wait.h
    Declarations for waiting for the events of children.

*******************************************************************************/

#ifndef INCLUDE_SORTIX_WAIT_H
#define INCLUDE_SORTIX_WAIT_H

#include <features.h>

__BEGIN_DECLS

#define WNOHANG (1<<0)

#define WEXITSTATUS(status) ((status >> 8) & 0xFF)
#define WTERMSIG(status) ((status >> 0) & 0x7F)
#define WSTOPSIG(status) WTERMSIG(status)
#define WIFEXITED(status) (WTERMSIG(status) == 0)
#define WIFSIGNALED(status) (WTERMSIG(status) != 0)
/*#define WIFCONTINUED(status) (WTERMSIG(status) == TODO)*/
/*#define WIFSTOPPED(status) (WTERMSIG(status) == TODO)*/
/*#define WIFCONTINUED(status) (WTERMSIG(status) == TODO)*/

#define W_EXITCODE(ret, sig) ((ret) << 8 | (sig))
#define W_STOPCODE(sig) ((sig) << 8 | 0x7f)

__END_DECLS

#endif
