/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

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

    ar.h
    Header describing the `ar' archive file format.

*******************************************************************************/

#ifndef INCLUDE_AR_H
#define INCLUDE_AR_H

#include <sys/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Archive files start with the ARMAG identifying string. Then follows a
   `struct ar_hdr', and as many bytes of member file data as its `ar_size'
   member indicates, for each member file. */

/* String that begins an archive file. */
#define ARMAG   "!<arch>\n"

/* Size of that string. */
#define SARMAG  8

/* String in ar_fmag at end of each header. */
#define ARFMAG  "`\n"

struct ar_hdr
{
	char ar_name[16];   /* Member file name, sometimes / terminated. */
	char ar_date[12];   /* File date, decimal seconds since Epoch. */
	char ar_uid[6];     /* User ID in ASCII decimal. */
	char ar_gid[6];     /* Group ID in ASCII decimal. */
	char ar_mode[8];    /* File mode, in ASCII octal. */
	char ar_size[10];   /* File size, in ASCII decimal. */
	char ar_fmag[2];    /* Always contains ARFMAG. */
};

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
