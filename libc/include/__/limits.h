/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2014.

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

    __/limits.h
    Implementation-defined constants.

*******************************************************************************/

#ifndef INCLUDE____LIMITS_H
#define INCLUDE____LIMITS_H

/* char */
#define __CHAR_BIT __CHAR_BIT__
#define __SCHAR_MIN (-__SCHAR_MAX - 1)
#define __SCHAR_MAX __SCHAR_MAX__
#if __SCHAR_MAX__ == __INT_MAX__
	#define __UCHAR_MAX (__SCHAR_MAX * 2U + 1U)
#else
	#define __UCHAR_MAX (__SCHAR_MAX * 2 + 1)
#endif
#ifdef __CHAR_UNSIGNED__
	#if __SCHAR_MAX__ == __INT_MAX__
		#define __CHAR_MIN 0U
	#else
		#define __CHAR_MIN 0
	#endif
	#define __CHAR_MAX __UCHAR_MAX
#else
	#define __CHAR_MIN __SCHAR_MIN
	#define __CHAR_MAX __SCHAR_MAX
#endif

/* short */
#define __SHRT_MIN (-__SHRT_MAX - 1)
#define __SHRT_MAX __SHRT_MAX__
#if __SHRT_MAX__ == __INT_MAX__
	#define __USHRT_MAX (__SHRT_MAX * 2U + 1U)
#else
	#define __USHRT_MAX (__SHRT_MAX * 2 + 1)
#endif

/* int */
#define __INT_MIN (-__INT_MAX - 1)
#define __INT_MAX __INT_MAX__
#define __UINT_MAX (__INT_MAX * 2U + 1U)

/* long */
#define __LONG_MIN (-__LONG_MAX - 1L)
#define __LONG_MAX __LONG_MAX__
#define __ULONG_MAX (__LONG_MAX * 2UL + 1UL)

/* long long */
#define __LLONG_MIN (-__LLONG_MAX - 1LL)
#define __LLONG_MAX __LONG_LONG_MAX__
#define __ULLONG_MAX (__LLONG_MAX * 2ULL + 1ULL)

#endif
