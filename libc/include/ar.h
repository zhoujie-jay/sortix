/*
 * Copyright (c) 2013 Jonas 'Sortie' Termansen.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * ar.h
 * Header describing the `ar' archive file format.
 */

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
