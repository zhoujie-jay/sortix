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

    pixel.c
    Pixel utilities.

*******************************************************************************/

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
