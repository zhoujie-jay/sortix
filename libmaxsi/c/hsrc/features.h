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

	features.h
	Various things for various systems, programs, compabillity, and whatnot.

******************************************************************************/

#ifndef	_FEATURES_H
#define	_FEATURES_H	1

/* C++ needs to know that types and declarations are C, not C++. */
#ifdef	__cplusplus
	#define __BEGIN_DECLS	extern "C" {
	#define __END_DECLS		}
#else
	#define __BEGIN_DECLS
	#define __END_DECLS
	#define restrict
#endif

#define __POSIX_NO_OBSOLETE

#ifdef __POSIX_NO_OBSOLETE
	#define __POSIX_OBSOLETE 999999L
#else
	#define __POSIX_OBSOLETE 200112L
#endif

#include <sortix/bits.h>

// Don't provide things from standard headers that is not implemented in libmaxsi/sortix.
#define SORTIX_UNIMPLEMENTED

#endif
