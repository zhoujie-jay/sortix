/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012, 2014, 2015.

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

    locale/setlocale.c
    Set program locale.

*******************************************************************************/

#include <errno.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>

static char* current_locales[LC_NUM_CATEGORIES] = { NULL };

char* setlocale(int category, const char* locale)
{
	if ( category < 0 || LC_ALL < category )
		return errno = EINVAL, (char*) NULL;
	char* new_strings[LC_NUM_CATEGORIES];
	int from = category != LC_ALL ? category : 0;
	int to = category != LC_ALL ? category : LC_NUM_CATEGORIES - 1;
	if ( !locale )
		return current_locales[to] ? current_locales[to] : (char*) "C";
	for ( int i = from; i <= to; i++ )
	{
		if ( !(new_strings[i] = strdup(locale)) )
		{
			for ( int n = from; n < i; n++ )
				free(new_strings[n]);
			return (char*) NULL;
		}
	}
	for ( int i = from; i <= to; i++ )
	{
		free(current_locales[i]);
		current_locales[i] = new_strings[i];
	}
	return (char*) locale;
}
