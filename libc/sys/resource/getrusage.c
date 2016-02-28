/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

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

    sys/resource/getrusage.c
    Get resource usage statistics.

*******************************************************************************/

#include <sys/resource.h>
#include <sys/syscall.h>

#include <errno.h>
#include <time.h>
#include <timespec.h>

static struct timeval timeval_of_timespec(struct timespec ts)
{
	struct timeval tv;
	tv.tv_sec = ts.tv_sec;
	tv.tv_usec = ts.tv_nsec / 1000;
	return tv;
}

int getrusage(int who, struct rusage* usage)
{
	struct tmns tmns;
	if ( timens(&tmns) != 0 )
		return -1;
	if ( who == RUSAGE_SELF )
	{
		usage->ru_utime = timeval_of_timespec(tmns.tmns_utime);
		usage->ru_stime = timeval_of_timespec(tmns.tmns_stime);
	}
	else if ( who == RUSAGE_CHILDREN )
	{
		usage->ru_utime = timeval_of_timespec(tmns.tmns_cutime);
		usage->ru_stime = timeval_of_timespec(tmns.tmns_cstime);
	}
	else
		return errno = EINVAL, -1;
	return 0;
}
