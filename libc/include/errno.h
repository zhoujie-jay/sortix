/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011.

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

    errno.h
    System error numbers.

*******************************************************************************/

#ifndef _ERRNO_H
#define _ERRNO_H 1

#include <features.h>

__BEGIN_DECLS

@include(errno_decl.h)
@include(errno_values.h)

extern char* program_invocation_name;
extern char* program_invocation_short_name;

/* Satisfy broken programs that expect these to be macros. */
#define program_invocation_name program_invocation_name
#define program_invocation_short_name program_invocation_short_name

__END_DECLS

#endif
