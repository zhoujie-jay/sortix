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

    pwd/fgetpwent_r.cpp
    Reads a passwd entry from a FILE.

*******************************************************************************/

#include <sys/types.h>

#include <errno.h>
#include <inttypes.h>
#include <pwd.h>
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

static id_t next_field_id(char** current)
{
	char* id_str = next_field(current);
	if ( !id_str )
		return -1;
	char* id_endptr;
	intmax_t id_imax = strtoimax(id_str, &id_endptr, 10);
	if ( id_imax < 0 || *id_endptr )
		return -1;
	id_t id = (id_t) id_imax;
	if ( id != id_imax )
		return -1;
	return id;
}

extern "C"
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
		return *result_pointer = NULL, errno = original_errno, NULL;
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
	if ( (result->pw_uid = next_field_id(&parse_str)) < 0 )
		goto parse_failure;
	if ( !(result->pw_gid = next_field_id(&parse_str)) < 0 )
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
