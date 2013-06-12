/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

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

    wctype.cpp
    Character types.

*******************************************************************************/

#include <string.h>
#include <wctype.h>

// TODO: Support other locales than ASCII.

extern "C" int iswalnum(wint_t c)
{
	return iswalpha(c) || iswdigit(c);
}

extern "C" int iswalpha(wint_t c)
{
	return iswupper(c) || iswlower(c);
}

extern "C" int iswblank(wint_t c)
{
	return c == L' ' || c == L'\t';
}

extern "C" int iswcntrl(wint_t c)
{
	return 0 <= c && c < 32;
}

extern "C" int iswdigit(wint_t c)
{
	return L'0' <= c && c <= L'9';
}

extern "C" int iswgraph(wint_t c)
{
	return L'!' <= c && c <= L'~';
}

extern "C" int iswlower(wint_t c)
{
	return L'a' <= c && c <= L'z';
}

extern "C" int iswprint(wint_t c)
{
	return iswgraph(c) || c == L' ';
}

extern "C" int iswpunct(wint_t c)
{
	return iswprint(c) && c != L' ' && !iswalnum(c);
}

extern "C" int iswspace(wint_t c)
{
	return c == L'\t' || c == L'\n' || c == L'\v' || c == L'\f' || c == L'\r' || c == L' ';
}

extern "C" int iswupper(wint_t c)
{
	return L'A' <= c && c <= L'Z';
}

extern "C" int iswxdigit(wint_t c)
{
	if ( iswdigit(c) ) { return 1; }
	if ( L'a' <= c && c <= L'f' ) { return 1; }
	if ( L'A' <= c && c <= L'F' ) { return 1; }
	return 0;
}

extern "C" int towlower(wint_t c)
{
	if ( L'A' <= c && c <= L'Z' ) { return L'a' + c - L'A'; }
	return c;
}

extern "C" int towupper(wint_t c)
{
	if ( L'a' <= c && c <= L'z' ) { return L'A' + c - L'a'; }
	return c;
}

extern "C" int iswctype(wint_t c, wctype_t wct)
{
	return wct(c);
}

extern "C" wctype_t wctype(const char *name)
{
	if ( !strcmp(name, "alnum") ) return (wctype_t) iswalnum;
	if ( !strcmp(name, "alpha") ) return (wctype_t) iswalpha;
	if ( !strcmp(name, "blank") ) return (wctype_t) iswblank;
	if ( !strcmp(name, "cntrl") ) return (wctype_t) iswcntrl;
	if ( !strcmp(name, "digit") ) return (wctype_t) iswdigit;
	if ( !strcmp(name, "graph") ) return (wctype_t) iswgraph;
	if ( !strcmp(name, "lower") ) return (wctype_t) iswlower;
	if ( !strcmp(name, "print") ) return (wctype_t) iswprint;
	if ( !strcmp(name, "punct") ) return (wctype_t) iswpunct;
	if ( !strcmp(name, "space") ) return (wctype_t) iswspace;
	if ( !strcmp(name, "upper") ) return (wctype_t) iswupper;
	if ( !strcmp(name, "xdigit") ) return (wctype_t) iswxdigit;
	return (wctype_t) 0;
}
