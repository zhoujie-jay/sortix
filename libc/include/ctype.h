/*
 * Copyright (c) 2011, 2014 Jonas 'Sortie' Termansen.
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
 * ctype.h
 * Character types.
 */

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
