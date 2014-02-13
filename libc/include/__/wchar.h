/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2014.

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

    __/wchar.h
    Wide character support.

*******************************************************************************/

#ifndef INCLUDE____WCHAR_H
#define INCLUDE____WCHAR_H

#include <sys/cdefs.h>

__BEGIN_DECLS

typedef __WCHAR_TYPE__ __wchar_t;
#define __WCHAR_MIN __WCHAR_MIN__
#define __WCHAR_MAX __WCHAR_MAX__

typedef __WINT_TYPE__ __wint_t;
#define __WINT_MIN __WINT_MIN__
#define __WINT_MAX __WINT_MAX__

#define __WEOF __WINT_MAX

__END_DECLS

#endif
