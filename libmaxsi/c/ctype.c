/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011.

	This file is part of LibMaxsi.

	LibMaxsi is free software: you can redistribute it and/or modify it under
	the terms of the GNU Lesser General Public License as published by the Free
	Software Foundation, either version 3 of the License, or (at your option)
	any later version.

	LibMaxsi is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
	more details.

	You should have received a copy of the GNU Lesser General Public License
	along with LibMaxsi. If not, see <http://www.gnu.org/licenses/>.

	ctype.c
	Character types.

******************************************************************************/

#include <ctype.h>

// TODO: Support other locales than ASCII.

int isalnum(int c)
{
	return isalpha(c) || isdigit(c);
}

int isalpha(int c)
{
	return isupper(c) || islower(c);
}

int isblank(int c)
{
	return c == ' ' || c == '\t';
}

int iscntrl(int c)
{
	return 0 <= c && c < 32;
}

int isdigit(int c)
{
	return '0' <= c && c <= '9';
}

int isgraph(int c)
{
	return '!' <= c && c <= '~';
}

int islower(int c)
{
	return 'a' <= c && c <= 'z';
}

int isprint(int c)
{
	return isgraph(c) || c == ' ';
}

int ispunct(int c)
{
	return isprint(c) && c != ' ' && !isalnum(c);
}

int isspace(int c)
{
	return c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r' || c == ' ';
}

int isupper(int c)
{
	return 'A' <= c && c <= 'Z';
}

int isxdigit(int c)
{
	if ( isdigit(c) ) { return 1; }
	if ( 'a' <= c && c <= 'f' ) { return 1; }
	if ( 'A' <= c && c <= 'F' ) { return 1; }
	return 0;
}

int tolower(int c)
{
	if ( 'A' <= c && c <= 'Z' ) { return 'a' + c - 'A'; }
	return c;
}

int toupper(int c)
{
	if ( 'a' <= c && c <= 'z' ) { return 'A' + c - 'a'; }
	return c;
}

