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

    pwent.cpp
    User database.

*******************************************************************************/

#include <sys/types.h>

#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// TODO: This implementation of pwent is very hacky and insecure.

const char* const ROOT_UID = "0";
const char* const ROOT_GID = "0";
const char* const ROOT_NAME = "root";
const char* const ROOT_HOME = "/root";
const char* const ROOT_SHELL = "sh";
const char* const ROOT_FULLNAME = "root";

static struct passwd global_passwd;

static size_t global_user_offset = 0;

extern "C" void setpwent(void)
{
	global_user_offset = 0;
}

extern "C" void endpwent(void)
{
}

static const char* getenv_def(const char* key, const char* def)
{
	const char* value = getenv(key);
	return value ? value : def;
}

static struct passwd* fill_passwd(struct passwd* pw)
{
	// TODO: Potential buffer overflow.
	strcpy(pw->pw_name, getenv_def("USERNAME", ROOT_NAME));
	strcpy(pw->pw_dir, getenv_def("HOME", ROOT_HOME));
	strcpy(pw->pw_shell, getenv_def("SHELL", ROOT_SHELL));
	strcpy(pw->pw_gecos, getenv_def("USERFULLNAME", ROOT_FULLNAME));
	pw->pw_uid = atoi(getenv_def("USERID", ROOT_UID));
	pw->pw_gid = atoi(getenv_def("GROUPID", ROOT_GID));
	return pw;
}

static uid_t lookup_username(const char* name)
{
	const char* my_username = getenv("USERNAME") ? getenv("USERNAME") : "root";
	if ( !strcmp(my_username, name) )
		if ( getenv("USERID") )
			return atoi(getenv("USERID"));
	return atoi(ROOT_UID);
}

extern "C" struct passwd* getpwent(void)
{
	if ( global_user_offset == 1 )
		return NULL;
	global_user_offset++;
	return fill_passwd(&global_passwd);
}

extern "C" struct passwd* getpwnam(const char* name)
{
	return getpwuid(lookup_username(name));
}

extern "C" int getpwnam_r(const char* name, struct passwd* pw, char* buf,
                          size_t buflen, struct passwd** ret)
{
	return getpwuid_r(lookup_username(name), pw, buf, buflen, ret);
}

extern "C" struct passwd* getpwuid(uid_t uid)
{
	struct passwd* ret;
	if ( getpwuid_r(uid, &global_passwd, NULL, 0, &ret) )
		return NULL;
	return ret;
}

extern "C" int getpwuid_r(uid_t uid, struct passwd* pw, char* buf,
                          size_t buflen, struct passwd** ret)
{
	(void) buf;
	(void) buflen;
	fill_passwd(pw);
	pw->pw_uid = uid;
	*ret = pw;
	return 0;
}
