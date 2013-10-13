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

    locale.h
    Category macros.

*******************************************************************************/

#ifndef _LOCALE_H
#define _LOCALE_H 1

#include <sys/cdefs.h>

#include <sys/__/types.h>

__BEGIN_DECLS

struct lconv
{
	char* decimal_point;
	char* thousands_sep;
	char* grouping;
	char* int_curr_symbol;
	char* currency_symbol;
	char* mon_decimal_point;
	char* mon_thousands_sep;
	char* mon_grouping;
	char* positive_sign;
	char* negative_sign;
	char int_frac_digits;
	char frac_digits;
	char p_cs_precedes;
	char n_cs_precedes;
	char p_sep_by_space;
	char n_sep_by_space;
	char p_sign_posn;
	char n_sign_posn;
	char int_p_cs_precedes;
	char int_n_cs_precedes;
	char int_p_sep_by_space;
	char int_n_sep_by_space;
	char int_p_sign_posn;
	char int_n_sign_posn;
};

#define LC_COLLATE 0
#define LC_CTYPE 1
#define LC_MESSAGES 2
#define LC_MONETARY 3
#define LC_NUMERIC 4
#define LC_TIME 5
#define LC_ALL 6
#define LC_NUM_CATEGORIES LC_ALL

const char* sortix_setlocale(int category, const char* locale);
#if defined(_SORTIX_SOURCE) && __SORTIX_STDLIB_REDIRECTS
const char* setlocale(int category, const char* locale) asm("sortix_setlocale");
#else
char* setlocale(int category, const char* locale);
#endif
struct lconv* localeconv(void);

__END_DECLS

#endif
