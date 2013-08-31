/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013.

    This file is part of the Sortix C Library.

    The Sortix C Library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    The Sortix C Library is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
    License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with the Sortix C Library. If not, see <http://www.gnu.org/licenses/>.

    stdio/fsetlocking_unlocked.cpp
    Sets the desired locking semantics on a FILE.

*******************************************************************************/

#include <stdio.h>

// TODO: What is this? This looks like something I ought to support!
extern "C" int fsetlocking_unlocked(FILE* fp, int type)
{
	switch ( type )
	{
	case FSETLOCKING_INTERNAL: fp->flags |= _FILE_AUTO_LOCK;
	case FSETLOCKING_BYCALLER: fp->flags &= ~_FILE_AUTO_LOCK;
	}
	return (fp->flags & _FILE_AUTO_LOCK) ? FSETLOCKING_INTERNAL
	                                     : FSETLOCKING_BYCALLER;
}
