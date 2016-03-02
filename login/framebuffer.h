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
 * framebuffer.h
 * Framebuffer utilities.
 */

#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <stddef.h>
#include <stdint.h>

struct framebuffer
{
	size_t pitch;
	uint32_t* buffer;
	size_t xres;
	size_t yres;
};

static inline uint32_t framebuffer_get_pixel(const struct framebuffer fb,
                                             size_t x,
                                             size_t y)
{
	if ( fb.xres <= x || fb.yres <= y )
		return 0;
	return fb.buffer[y * fb.pitch + x];
}

static inline void framebuffer_set_pixel(const struct framebuffer fb,
                                         size_t x,
                                         size_t y,
                                         uint32_t value)
{
	if ( fb.xres <= x || fb.yres <= y )
		return;
	fb.buffer[y * fb.pitch + x] = value;
}

struct framebuffer framebuffer_crop(struct framebuffer fb,
                                    size_t left,
                                    size_t top,
                                    size_t width,
                                    size_t height);
void framebuffer_copy_to_framebuffer(const struct framebuffer dst,
                                     const struct framebuffer src);
void framebuffer_copy_to_framebuffer_blend(const struct framebuffer dst,
                                           const struct framebuffer src);
struct framebuffer framebuffer_crop_int(struct framebuffer fb,
                                        int left,
                                        int top,
                                        int width,
                                        int height);
struct framebuffer framebuffer_cut_left_x(struct framebuffer fb, int offset);
struct framebuffer framebuffer_cut_right_x(struct framebuffer fb, int offset);
struct framebuffer framebuffer_cut_top_y(struct framebuffer fb, int offset);
struct framebuffer framebuffer_cut_bottom_y(struct framebuffer fb, int offset);
struct framebuffer framebuffer_center_x(struct framebuffer fb, int x, int width);
struct framebuffer framebuffer_center_y(struct framebuffer fb, int y, int height);
struct framebuffer framebuffer_right_x(struct framebuffer fb, int x, int width);
struct framebuffer framebuffer_bottom_y(struct framebuffer fb, int y, int height);
struct framebuffer framebuffer_center_text_x(struct framebuffer fb, int x, const char* str);
struct framebuffer framebuffer_center_text_y(struct framebuffer fb, int y, const char* str);
struct framebuffer framebuffer_right_text_x(struct framebuffer fb, int x, const char* str);
struct framebuffer framebuffer_bottom_text_y(struct framebuffer fb, int y, const char* str);

#endif
