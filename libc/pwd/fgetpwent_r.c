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

    pwd/fgetpwent_r.c
    Reads a passwd entry from a FILE.

*******************************************************************************/

#include <sys/types.h>

#include <errno.h>
#include <inttypes.h>
#include <pwd.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char* next_field(char** current)
{
	char* result = *current;
	if ( result )
	{
		char* next = result;
		while ( *next && *next != ':' )
			next++;
		if ( !*next )
			next = NULL;
		else
			*next++ = '\0';
		*current = next;
	}
	return result;
}

static bool next_field_uintmax(char** current, uintmax_t* result)
{
	char* id_str = next_field(current);
	if ( !id_str )
		return false;
	char* id_endptr;
	uintmax_t id_umax = strtoumax(id_str, &id_endptr, 10);
	if ( *id_endptr )
		return false;
	*result = id_umax;
	return true;
}

static gid_t next_field_gid(char** current, gid_t* result)
{
	uintmax_t id_umax;
	if ( !next_field_uintmax(current, &id_umax) )
		return false;
	gid_t gid = (gid_t) id_umax;
	if ( (uintmax_t) gid != (uintmax_t) id_umax )
		return false;
	*result = gid;
	return true;
}

static uid_t next_field_uid(char** current, uid_t* result)
{
	uintmax_t id_umax;
	if ( !next_field_uintmax(current, &id_umax) )
		return false;
	uid_t uid = (uid_t) id_umax;
	if ( (uintmax_t) uid != (uintmax_t) id_umax )
		return false;
	*result = uid;
	return true;
}

int fgetpwent_r(FILE* restrict fp,
                struct passwd* restrict result,
                char* restrict buf,
                size_t buf_len,
                struct passwd** restrict result_pointer)
{
	if ( !result_pointer )
		return errno = EINVAL;

	if ( !fp || !result || !buf )
		return *result_pointer = NULL, errno = EINVAL;

	int original_errno = errno;

	flockfile(fp);

	off_t original_offset = ftello(fp);
	if ( original_offset < 0 )
	{
		funlockfile(fp);
		return *result_pointer = NULL, errno;
	}

	size_t buf_used = 0;
	int ic;
	while ( (ic = fgetc(fp)) != EOF )
	{
		if ( ic == '\n' )
			break;

		if ( buf_used == buf_len )
		{
			fseeko(fp, original_offset, SEEK_SET);
			funlockfile(fp);
			return *result_pointer = NULL, errno = ERANGE;
		}

		buf[buf_used++] = (char) ic;
	}

	if ( ferror(fp) )
	{
		int original_error = errno;
		fseeko(fp, original_offset, SEEK_SET);
		funlockfile(fp);
		return *result_pointer = NULL, original_error;
	}

	if ( !buf_used && feof(fp) )
	{
		funlockfile(fp);
		return *result_pointer = NULL, errno = original_errno, 0;
	}

	if ( buf_used == buf_len )
	{
		fseeko(fp, original_offset, SEEK_SET);
		funlockfile(fp);
		return *result_pointer = NULL, errno = ERANGE;
	}
	buf[buf_used] = '\0';

	char* parse_str = buf;
	if ( !(result->pw_name = next_field(&parse_str)) )
		goto parse_failure;
	if ( !(result->pw_passwd = next_field(&parse_str)) )
		goto parse_failure;
	if ( !next_field_uid(&parse_str, &result->pw_uid) )
		goto parse_failure;
	if ( !next_field_gid(&parse_str, &result->pw_gid) )
		goto parse_failure;
	if ( !(result->pw_gecos = next_field(&parse_str)) )
		goto parse_failure;
	if ( !(result->pw_dir = next_field(&parse_str)) )
		goto parse_failure;
	if ( !(result->pw_shell = next_field(&parse_str)) )
		goto parse_failure;
	if ( parse_str )
		goto parse_failure;

	funlockfile(fp);

	return *result_pointer = result, 0;

parse_failure:
	fseeko(fp, original_offset, SEEK_SET);
	funlockfile(fp);
	return errno = EINVAL;
}
