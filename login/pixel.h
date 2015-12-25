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

    pixel.h
    Pixel utilities.

*******************************************************************************/

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
