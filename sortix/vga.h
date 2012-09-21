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

namespace Sortix {

const size_t VGA_FONT_WIDTH = 8UL;
const size_t VGA_FONT_HEIGHT = 16UL;
const size_t VGA_FONT_NUMCHARS = 256UL;
const size_t VGA_FONT_CHARSIZE = VGA_FONT_WIDTH * VGA_FONT_HEIGHT / 8UL;

namespace VGA {

void Init();
void SetCursor(unsigned x, unsigned y);
const uint8_t* GetFont();

} // namespace VGA

// TODO: This class shouldn't be exposed publicly; it is used in a hack in the
// /dev filesystem. However, vga.cpp should register /dev/vga instead.
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
	virtual ssize_t Read(uint8_t* dest, size_t count);
	virtual ssize_t Write(const uint8_t* src, size_t count);
	virtual bool IsReadable();
	virtual bool IsWritable();
	virtual size_t BlockSize();
	virtual uintmax_t Size();
	virtual uintmax_t Position();
	virtual bool Seek(uintmax_t position);
	virtual bool Resize(uintmax_t size);

};

} // namespace Sortix

#endif

