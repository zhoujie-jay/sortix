/*
 * Copyright (c) 2015 Jonas 'Sortie' Termansen.
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
 * fstab/scanfsent.c
 * Parse filesystem table entry.
 */

#include <ctype.h>
#include <fstab.h>
#include <stdlib.h>
#include <string.h>

static char* next_fsent_field(char** str_p)
{
	char* str = *str_p;
	while ( *str && isspace((unsigned char) *str) )
		str++;
	if ( !*str || *str == '#' )
		return NULL;
	size_t length = 1;
	while ( str[length] &&
	        !isspace((unsigned char) str[length]) &&
	        str[length] != '#' )
		length++;
	if ( str[length] )
	{
		char c = str[length];
		str[length] = '\0';
		*str_p = str + length + (c != '#' ? 1 : 0);
	}
	else
		*str_p = str + length;
	return str;
}

char* find_fstype(char* str)
{
	const char* types[] =
	{
		FSTAB_RO,
		FSTAB_RQ,
		FSTAB_RW,
		FSTAB_SW,
		FSTAB_XX,
	};
	size_t count = sizeof(types) / sizeof(types[0]);
	while ( *str )
	{
		while ( *str == ',' )
		{
			str++;
			continue;
		}
		size_t length = strcspn(str, ",");
		if ( length == 2 )
		{
			for ( size_t i = 0; i < count; i++ )
				if ( str[0] == types[i][0] && str[1] == types[i][1] )
					return (char*) types[i];
		}
		str += length;
	}
	return NULL;
}

int scanfsent(char* str, struct fstab* fs)
{
	char* str_freq;
	char* str_passno;
	if ( !(fs->fs_spec = next_fsent_field(&str)) ||
	     !(fs->fs_file = next_fsent_field(&str)) ||
	     !(fs->fs_vfstype = next_fsent_field(&str)) ||
	     !(fs->fs_mntops = next_fsent_field(&str)) ||
	     !(str_freq = next_fsent_field(&str)) ||
	     !(str_passno = next_fsent_field(&str)) )
		return 0;
	if ( !(fs->fs_type = find_fstype(fs->fs_mntops)) )
		return 0;
	if ( !strcmp(fs->fs_type, "xx") )
		return 0;
	fs->fs_freq = atoi(str_freq);
	fs->fs_passno = atoi(str_passno);
	return 1;
}
