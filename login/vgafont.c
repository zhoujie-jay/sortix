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

    vgafont.c
    VGA font.

*******************************************************************************/

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
