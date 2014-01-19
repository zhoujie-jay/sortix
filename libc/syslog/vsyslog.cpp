/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2014.

    This file is part of the Sortix C Library.

    The Sortix C Library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    The Sortix C Library is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
    License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with the Sortix C Library. If not, see <http://www.gnu.org/licenses/>.

    syslog/vsyslog.cpp
    Logs a condition to the system log.

*******************************************************************************/

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

extern "C" { char* __syslog_identity = NULL; }
extern "C" { int __syslog_facility = LOG_USER; }
extern "C" { int __syslog_fd = -1; }
extern "C" { int __syslog_mask = LOG_UPTO(LOG_DEBUG); }
extern "C" { int __syslog_option = 0; }

// TODO: The transmitted string might be delivered in chunks using this dprintf
//       approach (plus there are multiple dprintf calls). We should buffer the
//       string fully before transmitting it.

// TODO: The log entry format here is just whatever some other system does. We
//       should work out the Sortix system log semantics and adapt this to that.

extern "C" void vsyslog(int priority, const char* format, va_list ap)
{
	// Drop the event if it doesn't fit the current priority mask.
	if ( !(LOG_MASK(LOG_PRI(priority)) & __syslog_mask) )
		return;

	// Connect to the system log if a connection hasn't yet been established.
	if ( __syslog_fd < 0 && (__syslog_fd = connectlog()) < 0 )
		return;

	// If no facility is given we'll use the default facility from openlog.
	if ( !LOG_FAC(priority) )
		priority |= __syslog_facility;

	// Prepare a timestamp for the log event.
	struct timespec now;
	clock_gettime(CLOCK_REALTIME, &now);
	struct tm tm;
	gmtime_r(&now.tv_sec, &tm);
	char timestamp[32];
	strftime(timestamp, sizeof(timestamp), "%b %e %T", &tm);

	// Include the process id of the current process if requested.
	pid_t pid = (__syslog_option & LOG_PID) ? getpid() : 0;

	// Transmit the event to the system log.
	dprintf(__syslog_fd, "<%d>%s %s%s%.0jd%s: ",
	                     priority,
	                     timestamp,
	                     __syslog_identity ? __syslog_identity : "",
	                      "[" + !pid, (intmax_t) pid, "]" + !pid);
	vdprintf(__syslog_fd, format, ap);
	dprintf(__syslog_fd, "\n");
}
