/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

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

    sortix/kernel/user-timer.h
    Timer facility provided to user-space.

*******************************************************************************/

#ifndef INCLUDE_SORTIX_KERNEL_USER_TIMER_H
#define INCLUDE_SORTIX_KERNEL_USER_TIMER_H

#include <sys/types.h>

#include <sortix/sigevent.h>

#include <sortix/kernel/timer.h>

namespace Sortix {

class Process;

struct UserTimer
{
	Timer timer;
	struct sigevent event;
	Process* process;
	timer_t timerid;
};

} // namespace Sortix

#endif
