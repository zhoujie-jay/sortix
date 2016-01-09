/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013.

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

    sortix/kernel/time.h
    Retrieving the current time.

*******************************************************************************/

#ifndef INCLUDE_SORTIX_KERNEL_TIME_H
#define INCLUDE_SORTIX_KERNEL_TIME_H

#include <sys/cdefs.h>
#include <sys/types.h>

#include <sortix/timespec.h>

namespace Sortix {
class Clock;
class Process;
class Thread;
} // namespace Sortix

namespace Sortix {
namespace Time {

void Init();
void Start();
void OnTick(struct timespec tick_period, bool system_mode);
void InitializeProcessClocks(Process* process);
void InitializeThreadClocks(Thread* thread);
struct timespec Get(clockid_t clock);
Clock* GetClock(clockid_t clock);

} // namespace Time
} // namespace Sortix

#endif
