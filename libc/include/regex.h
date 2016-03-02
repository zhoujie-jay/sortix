/*
 * Copyright (c) 2014, 2015 Jonas 'Sortie' Termansen.
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
 * regex.h
 * Regular expressions.
 */

#ifndef _REGEX_H
#define _REGEX_H

#include <sys/cdefs.h>

#include <sys/__/types.h>

#if defined(__is_sortix_libc)
#include <pthread.h>
#else
#include <__/pthread.h>
#endif

#ifndef __size_t_defined
#define __size_t_defined
#define __need_size_t
#include <stddef.h>
#endif

typedef __ssize_t regoff_t;

typedef struct
{
	regoff_t rm_so;
	regoff_t rm_eo;
} regmatch_t;

#if defined(__is_sortix_libc)
enum re_type
{
	RE_TYPE_BOL,
	RE_TYPE_EOL,
	RE_TYPE_CHAR,
	RE_TYPE_ANY_CHAR,
	RE_TYPE_SET,
	RE_TYPE_SUBEXPRESSION,
	RE_TYPE_SUBEXPRESSION_END,
	RE_TYPE_ALTERNATIVE,
	RE_TYPE_OPTIONAL,
	RE_TYPE_LOOP,
	RE_TYPE_REPETITION,
	/* TODO: Back-references. */
};

struct re;

struct re_char
{
	char c;
};

struct re_set
{
	unsigned char set[32];
};

struct re_subexpression
{
	struct re* re_owner;
	size_t index;
};

struct re_split
{
	struct re* re;
	struct re* re_owner;
};

struct re_repetition
{
       struct re* re;
       size_t min;
       size_t max;
};

struct re
{
	enum re_type re_type;
	union
	{
		struct re_char re_char;
		struct re_set re_set;
		struct re_subexpression re_subexpression;
		struct re_split re_split;
		struct re_repetition re_repetition;
	};
	struct re* re_next;
	struct re* re_next_owner;
	struct re* re_current_state_prev;
	struct re* re_current_state_next;
	struct re* re_upcoming_state_next;
	unsigned char re_is_currently_done;
	unsigned char re_is_current;
	unsigned char re_is_upcoming;
	regmatch_t* re_matches;
};
#endif

typedef struct
{
	size_t re_nsub;
#if defined(__is_sortix_libc)
	pthread_mutex_t re_lock;
	struct re* re;
	regmatch_t* re_matches;
	size_t re_state_count;
	int re_cflags;
#else
	__pthread_mutex_t __re_lock;
	void* __re;
	regmatch_t* __re_matches;
	size_t __re_state_count;
	int __re_cflags;
#endif
} regex_t;

#define REG_EXTENDED (1 << 0)
#define REG_ICASE (1 << 1)
#define REG_NOSUB (1 << 2)
#define REG_NEWLINE (1 << 3)

#define REG_NOTBOL (1 << 0)
#define REG_NOTEOL (1 << 1)

#define REG_NOMATCH 1
#define REG_BADPAT 2
#define REG_ECOLLATE 3
#define REG_ECTYPE 4
#define REG_EESCAPE 5
#define REG_ESUBREG 6
#define REG_EBRACK 7
#define REG_EPAREN 8
#define REG_EBRACE 9
#define REG_BADBR 10
#define REG_ERANGE 11
#define REG_ESPACE 12
#define REG_BADRPT 13

#ifdef __cplusplus
extern "C" {
#endif

int regcomp(regex_t* __restrict, const char* __restrict, int);
size_t regerror(int, const regex_t* __restrict, char* __restrict, size_t);
int regexec(const regex_t* __restrict, const char* __restrict, size_t,
            regmatch_t* __restrict, int);
void regfree(regex_t*);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
