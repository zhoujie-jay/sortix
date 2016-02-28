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

    syslog/openlog.c
    Opens the connection to the system log.

*******************************************************************************/

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
