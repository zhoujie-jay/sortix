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

    com.h
    Handles communication to COM serial ports.

*******************************************************************************/

#ifndef SORTIX_COM_H
#define SORTIX_COM_H

namespace Sortix {

class Descriptor;

namespace COM {

void EarlyInit();
void Init(const char* devpath, Ref<Descriptor> slashdev);

} // namespace COM

} // namespace Sortix

#endif