/*******************************************************************************

	Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

	This file is part of LibMaxsi.

	LibMaxsi is free software: you can redistribute it and/or modify it under
	the terms of the GNU Lesser General Public License as published by the Free
	Software Foundation, either version 3 of the License, or (at your option)
	any later version.

	LibMaxsi is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
	details.

	You should have received a copy of the GNU Lesser General Public License
	along with LibMaxsi. If not, see <http://www.gnu.org/licenses/>.

	fsetlocking.cpp
	Sets the desired locking semantics on a FILE.

*******************************************************************************/

#include <stdio.h>

extern "C" int fsetlocking(FILE* fp, int type)
{
	switch ( type )
	{
	case FSETLOCKING_INTERNAL: fp->flags |= _FILE_AUTO_LOCK;
	case FSETLOCKING_BYCALLER: fp->flags &= ~_FILE_AUTO_LOCK;
	}
	return (fp->flags & _FILE_AUTO_LOCK) ? FSETLOCKING_INTERNAL
	                                     : FSETLOCKING_BYCALLER;
}
