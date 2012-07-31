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

	fregister.cpp
	Registers a FILE in the global list of open FILES, which allows the standard
	library to close and flush all open files upon program exit.

*******************************************************************************/

#include <stdio.h>

extern "C" { FILE* _firstfile = NULL; }

extern "C" void fregister(FILE* fp)
{
	fp->flags |= _FILE_REGISTERED;
	if ( !_firstfile ) { _firstfile = fp; return; }
	fp->next = _firstfile;
	_firstfile->prev = fp;
	_firstfile = fp;
}

extern "C" void funregister(FILE* fp)
{
	if ( !(fp->flags & _FILE_REGISTERED) ) { return; }
	if ( !fp->prev ) { _firstfile = fp->next; }
	if ( fp->prev ) { fp->prev->next = fp->next; }
	if ( fp->next ) { fp->next->prev = fp->prev; }
	fp->flags &= ~_FILE_REGISTERED;
}
