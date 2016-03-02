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
 * vgafont.c
 * VGA font.
 */

#include <fcntl.h>
#include <ioleast.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#include "framebuffer.h"
#include "vgafont.h"

unsigned char font[FONT_CHARSIZE * FONT_NUMCHARS];

bool load_font()
{
	static bool done = false;
	if ( done )
		return true;
	int fd = open("/dev/vgafont", O_RDONLY);
	if ( fd < 0 )
		return false;
	if ( readall(fd, font, sizeof(font)) != sizeof(font) )
		return false;
	close(fd);
	return done = true;
}

void render_char(struct framebuffer fb, char c, uint32_t color)
{
	unsigned char uc = (unsigned char) c;

	uint32_t buffer[FONT_HEIGHT * (FONT_WIDTH+1)];
	for ( size_t y = 0; y < FONT_HEIGHT; y++ )
	{
		unsigned char line_bitmap = font[uc * FONT_CHARSIZE + y];
		for ( size_t x = 0; x < FONT_WIDTH; x++ )
			buffer[y * (FONT_WIDTH+1) + x] = line_bitmap & 1U << (7 - x) ? color : 0;
		buffer[y * (FONT_WIDTH+1) + 8] = 0; //line_bitmap & 1U << 0 ? color : 0;
	}

	struct framebuffer character_fb;
	character_fb.xres = FONT_WIDTH + 1;
	character_fb.yres = FONT_HEIGHT;
	character_fb.pitch = character_fb.xres;
	character_fb.buffer = buffer;

	framebuffer_copy_to_framebuffer_blend(fb, character_fb);
}

void render_text(struct framebuffer fb, const char* str, uint32_t color)
{
	for ( size_t i = 0; str[i]; i++ )
		render_char(framebuffer_crop(fb, (FONT_WIDTH+1) * i, 0, fb.xres, fb.yres),
		            str[i], color);
}
