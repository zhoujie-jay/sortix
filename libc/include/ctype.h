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

    ctype.h
    Character types.

*******************************************************************************/

#ifndef INCLUDE_CTYPE_H
#define INCLUDE_CTYPE_H

#include <sys/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __locale_t_defined
#define __locale_t_defined
/* TODO: figure out what this does and typedef it properly. This is just a
         temporary assignment. */
typedef int __locale_t;
typedef __locale_t locale_t;
#endif

int isalnum(int c);
/* TODO: isalnum_l */
int isalpha(int c);
/* TODO: isalpha_l */
int isascii(int c);
/* TODO: isascii_l */
int isblank(int c);
/* TODO: isblank_l */
int iscntrl(int c);
/* TODO: iscntrl_l */
int isdigit(int c);
/* TODO: isdigit_l */
int isgraph(int c);
/* TODO: isgraph_l */
int islower(int c);
/* TODO: islower_l */
int isprint(int c);
/* TODO: isprint_l */
int ispunct(int c);
/* TODO: ispunct_l */
int isspace(int c);
/* TODO: isspace_l */
int isupper(int c);
/* TODO: isupper_l */
int isxdigit(int c);
/* TODO: isxdigit_l */
int tolower(int c);
/* TODO: tolower_l */
int toupper(int c);
/* TODO: toupper_l */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
