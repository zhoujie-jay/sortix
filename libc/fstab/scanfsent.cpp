/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2015.

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

    fstab/scanfsent.cpp
    Parse filesystem table entry.

*******************************************************************************/

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

extern "C" int scanfsent(char* str, struct fstab* fs)
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
