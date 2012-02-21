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

	stat.h
	Data returned by the stat() function.

******************************************************************************/

// TODO: Make this header comply with POSIX-1.2008

#ifndef	_STAT_H
#define	_STAT_H 1

#include <features.h>

__BEGIN_DECLS

@include(mode_t.h)
@include(mode_t_values.h)

int mkdir(const char *path, mode_t mode);

__END_DECLS

#endif
