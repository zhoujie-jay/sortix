/*******************************************************************************

	Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

	This file is part of Sortix.

	Sortix is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License as published by the Free Software
	Foundation, either version 3 of the License, or (at your option) any later
	version.

	Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
	details.

	You should have received a copy of the GNU General Public License along with
	Sortix. If not, see <http://www.gnu.org/licenses/>.

	vga.h
	A Video Graphics Array driver.

*******************************************************************************/

#ifndef SORTIX_VGA_H
#define SORTIX_VGA_H

#include "device.h"
#include "stream.h"

namespace Sortix
{
	const size_t VGA_FONT_WIDTH = 8UL;
	const size_t VGA_FONT_HEIGHT = 16UL;
	const size_t VGA_FONT_NUMCHARS = 256UL;
	const size_t VGA_FONT_CHARSIZE = VGA_FONT_WIDTH * VGA_FONT_HEIGHT / 8UL;

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

		void Init();
		void SetCursor(nat x, nat y);
		const uint8_t* GetFont();
	}

	class DevVGA : public DevBuffer
	{
	public:
		typedef DevBuffer BaseClass;

	public:
		DevVGA();
		virtual ~DevVGA();

	private:
		size_t offset;

	public:
		virtual ssize_t Read(byte* dest, size_t count);
		virtual ssize_t Write(const byte* src, size_t count);
		virtual bool IsReadable();
		virtual bool IsWritable();
		virtual size_t BlockSize();
		virtual uintmax_t Size();
		virtual uintmax_t Position();
		virtual bool Seek(uintmax_t position);
		virtual bool Resize(uintmax_t size);

	};
}

#endif

