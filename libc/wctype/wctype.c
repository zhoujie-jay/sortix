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

    wctype/wctype.c
    Returns the character class description with the given name.

*******************************************************************************/

#include <string.h>
#include <wctype.h>

wctype_t wctype(const char *name)
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
	return (wctype_t) NULL;
}
