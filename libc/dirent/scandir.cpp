/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2014.

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

    dirent/scandir.cpp
    Filtered and sorted directory reading.

*******************************************************************************/

#include <errno.h>
#include <dirent.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

static int wrap_filter(const struct dirent* dirent, void* function)
{
	return ((int (*)(const struct dirent*)) function)(dirent);
}

static int wrap_compare(const struct dirent** dirent_a,
                        const struct dirent** dirent_b, void* function)
{
	return ((int (*)(const struct dirent**, const struct dirent**)) function)(dirent_a, dirent_b);
}

extern "C"
int scandir(const char* path, struct dirent*** namelist_ptr,
            int (*filter)(const struct dirent*),
            int (*compare)(const struct dirent**, const struct dirent**))
{
	DIR* dir = opendir(path);
	if ( !dir )
		return -1;
	int (*used_filter)(const struct dirent*,
	                   void*) = filter ? wrap_filter : NULL;
	int (*used_compare)(const struct dirent**,
	                    const struct dirent**,
	                    void*) = compare ? wrap_compare : NULL;
	int result = dscandir_r(dir, namelist_ptr,
	                        used_filter, (void*) filter,
	                        used_compare, (void*) compare);
	closedir(dir);
	return result;
}
