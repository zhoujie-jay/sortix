/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2015, 2016.

    This program is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
    more details.

    You should have received a copy of the GNU General Public License along with
    this program. If not, see <http://www.gnu.org/licenses/>.

    release.h
    Utility functions to handle release information.

*******************************************************************************/

#ifndef RELEASE_H
#define RELEASE_H

struct release
{
	char* pretty_name;
	unsigned long version_major;
	unsigned long version_minor;
	bool version_dev;
	unsigned long abi_major;
	unsigned long abi_minor;
};

void release_free(struct release* release);
bool os_release_load(struct release* release,
                     const char* path,
                     const char* errpath);

#endif
