/*
 * Copyright (c) 2014 Jonas 'Sortie' Termansen.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * syslog/vsyslog.c
 * Logs a condition to the system log.
 */

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

char* __syslog_identity = NULL;
int __syslog_facility = LOG_USER;
int __syslog_fd = -1;
int __syslog_mask = LOG_UPTO(LOG_DEBUG);
int __syslog_option = 0;

// TODO: The transmitted string might be delivered in chunks using this dprintf
//       approach (plus there are multiple dprintf calls). We should buffer the
//       string fully before transmitting it.

// TODO: The log entry format here is just whatever some other system does. We
//       should work out the Sortix system log semantics and adapt this to that.

void vsyslog(int priority, const char* format, va_list ap)
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
