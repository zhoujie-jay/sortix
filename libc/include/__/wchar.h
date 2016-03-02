/*
 * Copyright (c) 2014 Jonas 'Sortie' Termansen.
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
 * __/wchar.h
 * Wide character support.
 */

#ifndef INCLUDE____WCHAR_H
#define INCLUDE____WCHAR_H

#include <sys/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef __WCHAR_TYPE__ __wchar_t;
#define __WCHAR_MIN __WCHAR_MIN__
#define __WCHAR_MAX __WCHAR_MAX__

typedef __WINT_TYPE__ __wint_t;
#define __WINT_MIN __WINT_MIN__
#define __WINT_MAX __WINT_MAX__

#define __WEOF __WINT_MAX

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
