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

    sortix/kernel/fsfunc.h
    Filesystem related utility functions.

*******************************************************************************/

#ifndef SORTIX_FSFUNC_H
#define SORTIX_FSFUNC_H

namespace Sortix {

static inline bool IsDotOrDotDot(const char* path)
{
	return path[0] == '.' && ((path[1] == '\0') ||
	                          (path[1] == '.' && path[2] == '\0'));
}

unsigned char ModeToDT(mode_t mode);
bool SplitFinalElem(const char* path, char** dir, char** final);

} // namespace Sortix

#endif
