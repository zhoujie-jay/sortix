/*******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2012.

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

	inttypes.h
	Fixed size integer types.

*******************************************************************************/

#ifndef	_INTTYPES_H
#define	_INTTYPES_H 1

#include <features.h>
#include <stdint.h>

/* TODO: This header does not fully comply with POSIX and the ISO C Standard. */

__BEGIN_DECLS

intmax_t imaxabs(intmax_t);

__END_DECLS

#endif
