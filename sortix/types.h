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

	types.h
	Declares the required datatypes for the x86 architecture.

******************************************************************************/

#ifndef SORTIX_TYPES_H
#define SORTIX_TYPES_H

// TODO: Declare stuff from <limits.h>.

#undef NULL
#if defined(__cplusplus)
	#define NULL 0
#else
	#define NULL ((void*) 0)
	typedef int wchar_t;
#endif

// Define basic signed types.
typedef char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef long long int64_t;

// Define basic unsigned types.
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

typedef unsigned int nat;

// The maximum width integers available on this platform.
typedef int64_t intmax_t;
typedef uint64_t uintmax_t;

// Define an integer able to hold the size of the largest continious memory
// region and define pointer safe integer types.
// TODO: 64-bit datatypes. Need to figure out which ifdef to use!
typedef unsigned int size_t;
typedef signed int ssize_t;
typedef signed int intptr_t;
typedef unsigned int uintptr_t;
typedef signed int ptrdiff_t;

#define INT8_MIN (0x80)
#define INT16_MIN (0x8000)
#define INT32_MIN (0x80000000)
#define INT64_MIN (0x8000000000000000ULL)
#define INT8_MAX (0x7F)
#define INT16_MAX (0x7FFF)
#define INT32_MAX (0x7FFFFFFF)
#define INT64_MAX (0x7FFFFFFFFFFFFFFFULL)
#define UINT8_MAX (0xFF)
#define UINT16_MAX (0xFFFF)
#define UINT32_MAX (0xFFFFFFFF)
#define UINT64_MAX (0xFFFFFFFFFFFFFFFFULL)

#define INTMAX_MIN INT64_MIN
#define INTMAX_MAX INT64_MAX
#define UINTMAX_MAX UINT64_MAX

#ifdef PLATFORM_X86
	#define SSIZE_MIN INT32_MIN
	#define SSIZE_MAX INT32_MAX
	#define SIZE_MAX UINT32_MAX
	#define INTPTR_MIN INT32_MIN
	#define INTPTR_MAX INT32_MAX
	#define UINTPTR_MAX UINT32_MAX
#elif defined(PLATFORM_X64)
	#define SSIZE_MIN INT64_MIN
	#define SSIZE_MAX INT64_MAX
	#define SIZE_MAX UINT64_MAX
	#define INTPTR_MIN INT64_MIN
	#define INTPTR_MAX INT64_MAX
	#define UINTPTR_MAX UINT64_MAX
#endif

typedef int wint_t;

#endif

