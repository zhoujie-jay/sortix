/*
 * Copyright (c) 2012, 2013, 2015 Jonas 'Sortie' Termansen.
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
 * assert/__assert.c
 * Reports the occurence of an assertion failure.
 */

#include <assert.h>
#include <scram.h>

#ifdef __is_sortix_libk
#include <libk.h>
#endif

void __assert(const char* filename,
              unsigned long line,
              const char* function_name,
              const char* expression)
{
#ifdef __is_sortix_libk
	libk_assert(filename, line, function_name, expression);
#else
	struct scram_assert info;
	info.filename = filename;
	info.line = line;
	info.function = function_name;
	info.expression = expression;
	scram(SCRAM_ASSERT, &info);
#endif
}
