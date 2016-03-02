/*
 * Copyright (c) 2013 Jonas 'Sortie' Termansen.
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
 * grp/fgetgrent_r.c
 * Reads a group entry from a FILE.
 */

#include <sys/types.h>

#include <errno.h>
#include <grp.h>
#include <inttypes.h>
#include <stdalign.h>
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

static size_t count_num_members(const char* member_string)
{
	size_t result = 0;
	while ( *member_string )
	{
		result++;
		while ( *member_string && *member_string != ',' )
			member_string++;
		if ( *member_string == ',' )
			member_string++;
	}
	return result;
}

static char* next_member(char** current)
{
	char* result = *current;
	if ( result )
	{
		char* next = result;
		while ( *next && *next != ',' )
			next++;
		if ( !*next )
			next = NULL;
		else
			*next++ = '\0';
		*current = next;
	}
	return result;
}

int fgetgrent_r(FILE* restrict fp,
                struct group* restrict result,
                char* restrict buf,
                size_t buf_len,
                struct group** restrict result_pointer)
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
	buf[buf_used++] = '\0';

	if ( false )
	{
	parse_failure:
		fseeko(fp, original_offset, SEEK_SET);
		funlockfile(fp);
		return errno = EINVAL;
	}

	if ( false )
	{
	range_failure:
		fseeko(fp, original_offset, SEEK_SET);
		funlockfile(fp);
		return *result_pointer = NULL, errno = ERANGE;
	}

	char* parse_str = buf;
	if ( !(result->gr_name = next_field(&parse_str)) )
		goto parse_failure;
	if ( !(result->gr_passwd = next_field(&parse_str)) )
		goto parse_failure;
	if ( !next_field_gid(&parse_str, &result->gr_gid) )
		goto parse_failure;
	char* member_string;
	if ( !(member_string = next_field(&parse_str)) )
		goto parse_failure;
	if ( parse_str )
		goto parse_failure;

	while ( buf_used % alignof(char*) != 0 )
	{
		if ( buf_used == buf_len )
			goto range_failure;
		buf_used++;
	}

	size_t num_members = count_num_members(member_string);
	size_t member_list_bytes = (num_members + 1) * sizeof(char*);
	size_t available_bytes = buf_len - buf_used;
	if ( available_bytes < member_list_bytes )
		goto range_failure;

	result->gr_mem = (char**) (buf + buf_used);

	char* member_parse_str = member_string;
	for ( size_t i = 0; i < num_members + 1; i++ )
		result->gr_mem[i] = next_member(&member_parse_str);

	funlockfile(fp);

	return *result_pointer = result, 0;
}
