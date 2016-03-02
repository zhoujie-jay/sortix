/*
 * Copyright (c) 2011, 2012 Jonas 'Sortie' Termansen.
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
 * sortix/vga.h
 * Standard symbolic constants and types for Sortix.
 */

#ifndef INC_SORTIX_VGA_H
#define INC_SORTIX_VGA_H

#include <sys/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

#define COLOR8_BLACK 0
#define COLOR8_BLUE 1
#define COLOR8_GREEN 2
#define COLOR8_CYAN 3
#define COLOR8_RED 4
#define COLOR8_MAGENTA 5
#define COLOR8_BROWN 6
#define COLOR8_LIGHT_GREY 7
#define COLOR8_DARK_GREY 8
#define COLOR8_LIGHT_BLUE 9
#define COLOR8_LIGHT_GREEN 10
#define COLOR8_LIGHT_CYAN 11
#define COLOR8_LIGHT_RED 12
#define COLOR8_LIGHT_MAGENTA 13
#define COLOR8_LIGHT_BROWN 14
#define COLOR8_WHITE 15

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
