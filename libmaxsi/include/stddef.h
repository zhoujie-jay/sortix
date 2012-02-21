/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011.

	This file is part of LibMaxsi.

	LibMaxsi is free software: you can redistribute it and/or modify it under
	the terms of the GNU Lesser General Public License as published by the Free
	Software Foundation, either version 3 of the License, or (at your option)
	any later version.

	LibMaxsi is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
	more details.

	You should have received a copy of the GNU Lesser General Public License
	along with LibMaxsi. If not, see <http://www.gnu.org/licenses/>.

	stddef.h
	Standard type definitions.

******************************************************************************/

#ifndef	_STDDEF_H
#define	_STDDEF_H 1

#include <features.h>

__BEGIN_DECLS

@include(NULL.h)

#define offsetof(type, member) __builtin_offsetof(type, member)

@include(ptrdiff_t.h)

@include(wchar_t.h)

@include(size_t.h)

__END_DECLS

#endif