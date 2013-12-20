/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012.

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

    utf8.h
    Encodes UTF-32 strings in UTF-8.

*******************************************************************************/

#ifndef SORTIX_UTF8_H
#define SORTIX_UTF8_H

namespace Sortix
{
	namespace UTF8
	{
		unsigned Encode(uint32_t unicode, char* dest);
	}
}

#endif
