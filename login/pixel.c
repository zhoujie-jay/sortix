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
 * pixel.c
 * Pixel utilities.
 */

#include <stdint.h>

#include "pixel.h"

uint32_t blend_pixel(uint32_t bg_value, uint32_t fg_value)
{
	union color_rgba8 fg; fg.value = fg_value;
	union color_rgba8 bg; bg.value = bg_value;
	if ( fg.a == 255 )
		return fg.value;
	if ( fg.a == 0 )
		return bg.value;
	union color_rgba8 ret;
	ret.a = 255;
	ret.r = ((255-fg.a)*bg.r + fg.a*fg.r) / 256;
	ret.g = ((255-fg.a)*bg.g + fg.a*fg.g) / 256;
	ret.b = ((255-fg.a)*bg.b + fg.a*fg.b) / 256;
	return ret.value;
}
