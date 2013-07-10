/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012.

    This file is part of dispd.

    dispd is free software: you can redistribute it and/or modify it under the
    terms of the GNU Lesser General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    dispd is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
    details.

    You should have received a copy of the GNU Lesser General Public License
    along with dispd. If not, see <http://www.gnu.org/licenses/>.

    framebuffer.h
    Keeps track of framebuffers.

*******************************************************************************/

#ifndef INCLUDE_DISPD_FRAMEBUFFER_H
#define INCLUDE_DISPD_FRAMEBUFFER_H

#if defined(__cplusplus)
extern "C" {
#endif

struct dispd_framebuffer
{
	struct dispd_window* window;
	uint8_t* data;
	size_t datasize;
	size_t pitch;
	int bpp;
	int width;
	int height;
	bool is_vga;
	bool is_rgba;
};

#if defined(__cplusplus)
} // extern "C"
#endif

#endif
