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
 * syslog/openlog.c
 * Opens the connection to the system log.
 */

#include <stdlib.h>
#include <string.h>
#include <syslog.h>

void openlog(const char* identity, int option, int facility)
{
	// Remember the identity to report in the system log events.
	free(__syslog_identity);
	__syslog_identity = identity ? strdup(identity) : (char*) NULL;

	// Remember the option and facility parameters for later use.
	__syslog_option = option;
	__syslog_facility = facility;

	// Connect to the system more immediately if we are asked to and need to.
	if ( (option & LOG_NDELAY) && __syslog_fd < 0 )
		__syslog_fd = connectlog();
}
