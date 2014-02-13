/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2014.

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

    wctype.h
    Wide-character classification and mapping utilities

*******************************************************************************/

#ifndef _WCTYPE_H
#define _WCTYPE_H 1

#include <sys/cdefs.h>

#include <sys/__/types.h>

#include <__/wchar.h>

__BEGIN_DECLS

#ifndef __wint_t_defined
#define __wint_t_defined
typedef __wint_t wint_t;
#endif

#ifndef __locale_t_defined
#define __locale_t_defined
/* TODO: figure out what this does and typedef it properly. This is just a
         temporary assignment. */
typedef int __locale_t;
typedef __locale_t locale_t;
#endif

#ifndef WEOF
#define WEOF __WEOF
#endif

/* TODO: Figure out what this does and typedef it properly. This is just a
         temporary assignment. */
typedef unsigned int wctrans_t;

typedef int (*wctype_t)(wint_t);

int iswalnum(wint_t);
int iswalpha(wint_t);
int iswblank(wint_t);
int iswcntrl(wint_t);
int iswctype(wint_t, wctype_t);
int iswdigit(wint_t);
int iswgraph(wint_t);
int iswlower(wint_t);
int iswprint(wint_t);
int iswpunct(wint_t);
int iswspace(wint_t);
int iswupper(wint_t);
int iswxdigit(wint_t);
wint_t towlower(wint_t);
wint_t towupper(wint_t);
wctype_t wctype(const char *);

/* TODO: These are not implemented in sortix libc yet. */
#if 0
int iswalnum_l(wint_t, locale_t);
int iswalpha_l(wint_t, locale_t);
int iswblank_l(wint_t, locale_t);
int iswcntrl_l(wint_t, locale_t);
int iswctype_l(wint_t, wctype_t, locale_t);
int iswdigit_l(wint_t, locale_t);
int iswgraph_l(wint_t, locale_t);
int iswlower_l(wint_t, locale_t);
int iswprint_l(wint_t, locale_t);
int iswpunct_l(wint_t, locale_t);
int iswspace_l(wint_t, locale_t);
int iswupper_l(wint_t, locale_t);
int iswxdigit_l(wint_t, locale_t);
wint_t towctrans(wint_t, wctrans_t);
wint_t towctrans_l(wint_t, wctrans_t, locale_t);
wint_t towlower_l(wint_t, locale_t);
wint_t towupper_l(wint_t, locale_t);
wctrans_t wctrans(const char *);
wctrans_t wctrans_l(const char *, locale_t);
wctype_t wctype_l(const char *, locale_t);
#endif

__END_DECLS

#endif
