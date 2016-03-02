/*
 * Copyright (c) 2015 Jonas 'Sortie' Termansen.
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
 * scram.h
 * Emergency process shutdown.
 */

#ifndef _SCRAM_H
#define _SCRAM_H

#include <sys/cdefs.h>

#define SCRAM_ASSERT 1
#define SCRAM_STACK_SMASH 2
#define SCRAM_UNDEFINED_BEHAVIOR 3

struct scram_assert
{
	const char* filename;
	unsigned long line;
	const char* function;
	const char* expression;
};

struct scram_undefined_behavior
{
	const char* filename;
	unsigned long line;
	unsigned long column;
	const char* violation;
};

#ifdef __cplusplus
extern "C" {
#endif

__attribute__((noreturn))
void scram(int, const void*);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
