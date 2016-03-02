/*
 * Copyright (c) 2011, 2012, 2013, 2014, 2015 Jonas 'Sortie' Termansen.
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
 * util.h
 * Utility functions.
 */

#ifndef UTIL_H
#define UTIL_H

char* strdup_safe(const char* string);
const char* getenv_safe_def(const char* name, const char* def);
const char* getenv_safe(const char* name);
bool array_add(void*** array_ptr,
               size_t* used_ptr,
               size_t* length_ptr,
               void* value);
bool might_need_shell_quote(char c);

struct stringbuf
{
	char* string;
	size_t length;
	size_t allocated_size;
};

void stringbuf_begin(struct stringbuf* buf);
void stringbuf_append_c(struct stringbuf* buf, char c);
char* stringbuf_finish(struct stringbuf* buf);

#endif
