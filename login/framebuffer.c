/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2014, 2015.

    This program is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
    more details.

    You should have received a copy of the GNU General Public License along with
    this program. If not, see <http://www.gnu.org/licenses/>.

    framebuffer.c
    Framebuffer utilities.

*******************************************************************************/

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "framebuffer.h"
#include "pixel.h"
#include "vgafont.h"

struct framebuffer framebuffer_crop(struct framebuffer fb,
                                    size_t left,
                                    size_t top,
                                    size_t width,
                                    size_t height)
{
	// Crop the framebuffer horizontally.
	if ( fb.xres < left )
		left = fb.xres;
	fb.buffer += left;
	fb.xres -= left;
	if ( width < fb.xres )
		fb.xres = width;

	// Crop the framebuffer vertically.
	if ( fb.yres < top )
		top = fb.yres;
	fb.buffer += top * fb.pitch;
	fb.yres -= top;
	if ( height < fb.yres )
		fb.yres = height;

	return fb;
}

void framebuffer_copy_to_framebuffer(const struct framebuffer dst,
                                     const struct framebuffer src)
{
	for ( size_t y = 0; y < src.yres; y++ )
		for ( size_t x = 0; x < src.xres; x++ )
			framebuffer_set_pixel(dst, x, y, framebuffer_get_pixel(src, x, y));
}

void framebuffer_copy_to_framebuffer_blend(const struct framebuffer dst,
                                           const struct framebuffer src)
{
	for ( size_t y = 0; y < src.yres; y++ )
	{
		for ( size_t x = 0; x < src.xres; x++ )
		{
			uint32_t bg = framebuffer_get_pixel(dst, x, y);
			uint32_t fg = framebuffer_get_pixel(src, x, y);
			framebuffer_set_pixel(dst, x, y, blend_pixel(bg, fg));
		}
	}
}

struct framebuffer framebuffer_crop_int(struct framebuffer fb,
                                        int left,
                                        int top,
                                        int width,
                                        int height)
{
	if ( left < 0 ) { width -= -left; left = 0; }
	if ( top < 0 ) { top -= -height; top -= 0; }
	if ( width < 0 ) { width = 0; }
	if ( height < 0 ) { height = 0; }
	return framebuffer_crop(fb, left, top, width, height);
}

struct framebuffer framebuffer_cut_left_x(struct framebuffer fb, int offset)
{
	fb = framebuffer_crop_int(fb, offset, 0, fb.xres - offset, fb.yres);
	return fb;
}

struct framebuffer framebuffer_cut_right_x(struct framebuffer fb, int offset)
{
	fb = framebuffer_crop_int(fb, 0, 0, fb.xres - offset, fb.yres);
	return fb;
}

struct framebuffer framebuffer_cut_top_y(struct framebuffer fb, int offset)
{
	fb = framebuffer_crop_int(fb, 0, offset, fb.xres, fb.yres - offset);
	return fb;
}

struct framebuffer framebuffer_cut_bottom_y(struct framebuffer fb, int offset)
{
	fb = framebuffer_crop_int(fb, 0, 0, fb.xres, fb.yres - offset);
	return fb;
}

struct framebuffer framebuffer_center_x(struct framebuffer fb, int x, int width)
{
	x = x - width / 2;
	if ( x < 0 ) { width -= -x; x = 0; }
	if ( width < 0 ) { width = 0; }
	fb = framebuffer_crop(fb, x, 0, width, fb.yres);
	return fb;
}

struct framebuffer framebuffer_center_y(struct framebuffer fb, int y, int height)
{
	y = y - height / 2;
	if ( y < 0 ) { height -= -y; y = 0; }
	if ( height < 0 ) { height = 0; }
	fb = framebuffer_crop(fb, 0, y, fb.xres, height);
	return fb;
}

struct framebuffer framebuffer_right_x(struct framebuffer fb, int x, int width)
{
	x = x - width;
	if ( x < 0 ) { width -= -x; x = 0; }
	if ( width < 0 ) { width = 0; }
	fb = framebuffer_crop(fb, x, 0, width, fb.yres);
	return fb;
}

struct framebuffer framebuffer_bottom_y(struct framebuffer fb, int y, int height)
{
	y = y - height;
	if ( y < 0 ) { height -= -y; y = 0; }
	if ( height < 0 ) { height = 0; }
	fb = framebuffer_crop(fb, 0, y, fb.xres, height);
	return fb;
}

struct framebuffer framebuffer_center_text_x(struct framebuffer fb, int x, const char* str)
{
	int width = (FONT_WIDTH + 1) * strlen(str);
	return framebuffer_center_x(fb, x, width);
}

struct framebuffer framebuffer_center_text_y(struct framebuffer fb, int y, const char* str)
{
	(void) str;
	int height = FONT_HEIGHT;
	return framebuffer_center_y(fb, y, height);
}

struct framebuffer framebuffer_right_text_x(struct framebuffer fb, int x, const char* str)
{
	int width = (FONT_WIDTH + 1) * strlen(str);
	return framebuffer_right_x(fb, x, width);
}

struct framebuffer framebuffer_bottom_text_y(struct framebuffer fb, int y, const char* str)
{
	(void) str;
	int height = FONT_HEIGHT;
	return framebuffer_bottom_y(fb, y, height);
}
