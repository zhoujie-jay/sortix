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

    grent.cpp
    Group database.

*******************************************************************************/

#include <sys/types.h>

#include <grp.h>
#include <stdlib.h>
#include <string.h>

const gid_t ROOT_GID = 0;
const char* const ROOT_NAME = "root";

static struct group global_group;

static struct group* fill_group(struct group* gr)
{
	const char* env_groupname = getenv("GROUPNAME");
	strcpy(gr->gr_name, env_groupname ? env_groupname : ROOT_NAME);
	const char* env_groupid = getenv("GROUPID");
	gr->gr_gid = env_groupid ? atoi(env_groupid) : ROOT_GID;
	return gr;
}

static gid_t lookup_groupname(const char* name)
{
	const char* env_groupname = getenv("GROUPNAME");
	const char* my_groupname = env_groupname ? env_groupname : ROOT_NAME;
	if ( !strcmp(my_groupname, name) )
	{
		const char* env_groupid = getenv("GROUPID");
		if ( env_groupid )
			return atoi(env_groupid);
	}
	return 1;
}

extern "C" struct group* getgrgid(gid_t gid)
{
	(void) gid;
	return fill_group(&global_group);
}

extern "C" struct group* getgrnam(const char* name)
{
	return getgrgid(lookup_groupname(name));
}
