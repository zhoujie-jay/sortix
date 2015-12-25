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

    vgafont.h
    VGA font.

*******************************************************************************/

#ifndef VGAFONT_H
#define VGAFONT_H

#include <stdint.h>

#include "framebuffer.h"

#define FONT_WIDTH 8
#define FONT_HEIGHT 16
#define FONT_NUMCHARS 256
#define FONT_CHARSIZE ((FONT_WIDTH * FONT_HEIGHT) / 8)

extern uint8_t font[FONT_CHARSIZE * FONT_NUMCHARS];

bool load_font();
void render_char(struct framebuffer fb, char c, uint32_t color);
void render_text(struct framebuffer fb, const char* str, uint32_t color);

#endif
