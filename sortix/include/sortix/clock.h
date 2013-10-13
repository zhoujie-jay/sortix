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

    sortix/clock.h
    Supported logical clock devices.

*******************************************************************************/

#ifndef INCLUDE_SORTIX_CLOCK_H
#define INCLUDE_SORTIX_CLOCK_H

#include <sys/cdefs.h>

__BEGIN_DECLS

#define CLOCK_REALTIME 0 /* Current real time. */
#define CLOCK_MONOTONIC 1 /* Always increasing time. */
#define CLOCK_BOOT 2 /* Time since system boot (uptime). */
#define CLOCK_INIT 3 /* Time since 'init' process began. */
#define CLOCK_PROCESS_CPUTIME_ID 4
#define CLOCK_PROCESS_SYSTIME_ID 5
#define CLOCK_CHILD_CPUTIME_ID 6
#define CLOCK_CHILD_SYSTIME_ID 7
#define CLOCK_THREAD_CPUTIME_ID 8
#define CLOCK_THREAD_SYSTIME_ID 9

__END_DECLS

#endif
