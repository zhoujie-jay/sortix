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
 * pixel.h
 * Pixel utilities.
 */

#ifndef PIXEL_H
#define PIXEL_H

#include <stdint.h>

// TODO: This isn't the only pixel format in the world!
union color_rgba8
{
	struct
	{
		uint8_t b;
		uint8_t g;
		uint8_t r;
		uint8_t a;
	};
	uint32_t value;
};

__attribute__((used))
static inline uint32_t make_color_a(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	union color_rgba8 color;
	color.r = r;
	color.g = g;
	color.b = b;
	color.a = a;
	return color.value;
}

__attribute__((used))
static inline uint32_t make_color(uint8_t r, uint8_t g, uint8_t b)
{
	return make_color_a(r, g, b, 255);
}

uint32_t blend_pixel(uint32_t bg_value, uint32_t fg_value);

#endif
