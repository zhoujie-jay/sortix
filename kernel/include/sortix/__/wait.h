/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2014.

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

    sortix/__/wait.h
    Declarations for waiting for the events of children.

*******************************************************************************/

#ifndef INCLUDE_SORTIX____WAIT_H
#define INCLUDE_SORTIX____WAIT_H

#include <sys/cdefs.h>

__BEGIN_DECLS

#define __WNATURE_EXITED 0
#define __WNATURE_SIGNALED 1
#define __WNATURE_STOPPED 2
#define __WNATURE_CONTINUED 3

#define __WEXITSTATUS(status) (((status) >> 8) & 0xFF)
#define __WTERMSIG(status) (((status) >> 0) & 0x7F)
#define __WNATURE(status) (((status) >> 16) & 0xFF)

#define __WIFEXITED(status) (__WNATURE(status) == __WNATURE_EXITED)
#define __WIFSIGNALED(status) (__WNATURE(status) == __WNATURE_SIGNALED)
#define __WIFSTOPPED(status) (__WNATURE(status) == __WNATURE_STOPPED)
#define __WIFCONTINUED(status) (__WNATURE(status) == __WNATURE_CONTINUED)

#define __WSTOPSIG(status) __WTERMSIG(status)

#define __WCONSTRUCT(nature, exitcode, signal) \
        (((nature) & 0xFF) << 16 | \
         ((exitcode) & 0xFF) << 8 | \
         ((signal) & 0x7F) << 0)

__END_DECLS

#endif
