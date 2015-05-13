/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012, 2014.

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

#include <sys/cdefs.h>

#include <sortix/__/wait.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WCONTINUED (1<<0)
#define WNOHANG (1<<1)
#define WUNTRACED (1<<2)

#define WNATURE_EXITED __WNATURE_EXITED
#define WNATURE_SIGNALED __WNATURE_SIGNALED
#define WNATURE_STOPPED __WNATURE_STOPPED
#define WNATURE_CONTINUED __WNATURE_CONTINUED

#define WEXITSTATUS(status) __WEXITSTATUS(status)
#define WTERMSIG(status) __WTERMSIG(status)
#define WNATURE(status) __WNATURE(status)

#define WIFEXITED(status) __WIFEXITED(status)
#define WIFSIGNALED(status) __WIFSIGNALED(status)
#define WIFSTOPPED(status) __WIFSTOPPED(status)
#define WIFCONTINUED(status) __WIFCONTINUED(status)

#define WSTOPSIG(status) __WSTOPSIG(status)

#define WCONSTRUCT(nature, exitcode, signal) \
        __WCONSTRUCT(nature, exitcode, signal)

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
