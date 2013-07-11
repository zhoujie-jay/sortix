/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011.

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

    ctype/ctype.cpp
    Character types.

*******************************************************************************/

#include <ctype.h>

// TODO: Support other locales than ASCII.

extern "C" int isalnum(int c)
{
	return isalpha(c) || isdigit(c);
}

extern "C" int isalpha(int c)
{
	return isupper(c) || islower(c);
}

extern "C" int isblank(int c)
{
	return c == ' ' || c == '\t';
}

extern "C" int iscntrl(int c)
{
	return 0 <= c && c < 32;
}

extern "C" int isdigit(int c)
{
	return '0' <= c && c <= '9';
}

extern "C" int isgraph(int c)
{
	return '!' <= c && c <= '~';
}

extern "C" int islower(int c)
{
	return 'a' <= c && c <= 'z';
}

extern "C" int isprint(int c)
{
	return isgraph(c) || c == ' ';
}

extern "C" int ispunct(int c)
{
	return isprint(c) && c != ' ' && !isalnum(c);
}

extern "C" int isspace(int c)
{
	return c == '\t' || c == '\n' || c == '\v' ||
	       c == '\f' || c == '\r' || c == ' ';
}

extern "C" int isupper(int c)
{
	return 'A' <= c && c <= 'Z';
}

extern "C" int isxdigit(int c)
{
	if ( isdigit(c) )
		return 1;
	if ( 'a' <= c && c <= 'f' ) return 1;
	if ( 'A' <= c && c <= 'F' ) return 1;
	return 0;
}

extern "C" int tolower(int c)
{
	if ( 'A' <= c && c <= 'Z' )
		return 'a' + c - 'A';
	return c;
}

extern "C" int toupper(int c)
{
	if ( 'a' <= c && c <= 'z' )
		return 'A' + c - 'a';
	return c;
}
