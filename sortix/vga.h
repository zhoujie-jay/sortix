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

#include <sortix/kernel/refcount.h>

namespace Sortix {

class Descriptor;

const size_t VGA_FONT_WIDTH = 8UL;
const size_t VGA_FONT_HEIGHT = 16UL;
const size_t VGA_FONT_NUMCHARS = 256UL;
const size_t VGA_FONT_CHARSIZE = VGA_FONT_WIDTH * VGA_FONT_HEIGHT / 8UL;

namespace VGA {

void Init(const char* devpath, Ref<Descriptor> slashdev);
void SetCursor(unsigned x, unsigned y);
const uint8_t* GetFont();

} // namespace VGA

} // namespace Sortix

#endif
