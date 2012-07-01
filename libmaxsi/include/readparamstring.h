/*******************************************************************************

	Copyright(C) Jonas 'Sortie' Termansen 2012.

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

	readparamstring.h
	A kinda dirty hack to ease transition to the /dev/video API.

*******************************************************************************/

#ifndef _READPARAMSTRING_H
#define _READPARAMSTRING_H 1

#include <features.h>

__BEGIN_DECLS

bool ReadParamString(const char* str, ...);

__END_DECLS

#endif
