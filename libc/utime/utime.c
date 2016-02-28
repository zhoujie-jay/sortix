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

    utime/utime.c
    Change file last access and modification times.

*******************************************************************************/

#include <sys/stat.h>
#include <sys/types.h>

#include <utime.h>
#include <time.h>
#include <timespec.h>

// TODO: This interface has been deprecated by utimens (although that's a
//       non-standard Sortix extension, in which case by utimensat) - it'd be
//       nice to remove this at some point.
int utime(const char* filepath, const struct utimbuf* times)
{
	struct timespec ts_times[2];
	ts_times[0] = timespec_make(times->actime, 0);
	ts_times[1] = timespec_make(times->modtime, 0);
	return utimens(filepath, ts_times);
}
