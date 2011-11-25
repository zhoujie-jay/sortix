/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011.

	This file is part of LibMaxsi.

	LibMaxsi is free software: you can redistribute it and/or modify it under
	the terms of the GNU Lesser General Public License as published by the Free
	Software Foundation, either version 3 of the License, or (at your option)
	any later version.

	LibMaxsi is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
	more details.

	You should have received a copy of the GNU Lesser General Public License
	along with LibMaxsi. If not, see <http://www.gnu.org/licenses/>.

	sortix-vga.h
	Provides access to the VGA framebuffer under Sortix. This is highly
	unportable and is very likely to be removed or changed radically.

******************************************************************************/

#ifndef LIBMAXSI_SORTIX_VGA_H
#define LIBMAXSI_SORTIX_VGA_H

namespace System
{
	namespace VGA
	{
		// TODO: Move these to a better place
		#define COLOR8_BLACK 0
		#define COLOR8_BLUE 1
		#define COLOR8_GREEN 2
		#define COLOR8_CYAN 3
		#define COLOR8_RED 4
		#define COLOR8_MAGENTA 5
		#define COLOR8_BROWN 6
		#define COLOR8_LIGHT_GREY 7
		#define COLOR8_DARK_GREY 8
		#define COLOR8_LIGHT_BLUE 9
		#define COLOR8_LIGHT_GREEN 10
		#define COLOR8_LIGHT_CYAN 11
		#define COLOR8_LIGHT_RED 12
		#define COLOR8_LIGHT_MAGENTA 13
		#define COLOR8_LIGHT_BROWN 14
		#define COLOR8_WHITE 15
	}
}

#endif
