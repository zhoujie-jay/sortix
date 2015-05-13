/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

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

    stdlib/strtoull.cpp
    Converts integers represented as strings to binary representation.

*******************************************************************************/

#define STRTOL strtoull
#define STRTOL_CHAR char
#define STRTOL_UCHAR unsigned char
#define STRTOL_L(x) x
#define STRTOL_ISSPACE isspace
#define STRTOL_INT unsigned long long
#define STRTOL_UNSIGNED_INT unsigned long long
#define STRTOL_INT_MIN 0
#define STRTOL_INT_MAX ULLONG_MAX
#define STRTOL_INT_IS_UNSIGNED true

#include "strtol.cpp"
