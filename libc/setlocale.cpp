/*******************************************************************************

	Copyright(C) Jonas 'Sortie' Termansen 2012.

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

	setlocale.cpp
	Set program locale.

*******************************************************************************/

#define __SORTIX_STDLIB_REDIRECTS 0
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <errno.h>

static char* current_locales[LC_NUM_CATEGORIES] = { NULL };

extern "C" const char* sortix_setlocale(int category, const char* locale)
{
	if ( category < 0 || LC_ALL < category ) { errno = EINVAL; return NULL; }
	char* new_strings[LC_NUM_CATEGORIES];
	int from = category;
	int to = category;
	if ( !locale ) { return current_locales[to]; }
	if ( category == LC_ALL ) { from = 0; to = LC_NUM_CATEGORIES-1; }
	for ( int i = from; i <= to; i++ )
	{
		new_strings[i] = strdup(locale);
		if ( !new_strings[i] )
		{
			for ( int n = from; n < i; n++ ) { free(new_strings[n]); }
			return NULL;
		}
	}
	for ( int i = from; i <= to; i++ )
	{
		free(current_locales[i]);
		current_locales[i] = new_strings[i];
	}
	return locale;
}

extern "C" char* setlocale(int category, const char* locale)
{
	return (char*) sortix_setlocale(category, locale);
}

