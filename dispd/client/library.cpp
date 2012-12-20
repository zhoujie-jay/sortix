/*******************************************************************************

	Copyright(C) Jonas 'Sortie' Termansen 2012.

	This file is part of dispd.

	dispd is free software: you can redistribute it and/or modify it under the
	terms of the GNU Lesser General Public License as published by the Free
	Software Foundation, either version 3 of the License, or (at your option)
	any later version.

	dispd is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
	details.

	You should have received a copy of the GNU Lesser General Public License
	along with dispd. If not, see <http://www.gnu.org/licenses/>.

	library.cpp
	Main entry point of the Sortix Display Daemon.

*******************************************************************************/

#include <stddef.h>
#include <stdint.h>

#include <dispd.h>

#include "session.h"

extern "C" bool dispd_initialize(int* argc, char*** argv)
{
	if ( !dispd__session_initialize(argc, argv) )
		return false;
	return true;
}