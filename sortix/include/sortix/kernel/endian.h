/*******************************************************************************

	Copyright(C) Jonas 'Sortie' Termansen 2012.

	This file is part of Sortix.

	Sortix is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License as published by the Free Software
	Foundation, either version 3 of the License, or (at your option) any later
	version.

	Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
	details.

	You should have received a copy of the GNU General Public License along with
	Sortix. If not, see <http://www.gnu.org/licenses/>.

	endian.h
	Converting values to and from various endians.

*******************************************************************************/

#ifndef SORTIX_ENDIAN_H
#define SORTIX_ENDIAN_H

// Work around older GCC versions that do not define this.
#if !defined(__BYTE_ORDER__) && (defined(__i386__) || defined(__x86_64__))
#if !defined(__ORDER_LITTLE_ENDIAN__)
#define __ORDER_LITTLE_ENDIAN__ 1234
#endif
#if !defined(__ORDER_BIG_ENDIAN__)
#define __ORDER_BIG_ENDIAN__ 4321
#endif
#if !defined(__BYTE_ORDER__)
#define __BYTE_ORDER__ __ORDER_LITTLE_ENDIAN__
#endif
#endif

#ifndef __BYTE_ORDER__
#error Your compiler needs to define the __BYTE_ORDER__ macro family.
#endif

#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__ && \
    __BYTE_ORDER__ != __ORDER_BIG_ENDIAN__
#error Exotic endian detected, you need to port this header.
#endif

namespace Sortix {

static inline uint16_t SwapBytes(uint16_t val)
{
	uint16_t a = val >> 0 & 0xFF;
	uint16_t b = val >> 8 & 0xFF;
	return a <<  8U | b <<  0U;
}

static inline uint32_t SwapBytes(uint32_t val)
{
	uint32_t a = val >> 0 & 0xFF;
	uint32_t b = val >> 8 & 0xFF;
	uint32_t c = val >> 16 & 0xFF;
	uint32_t d = val >> 24 & 0xFF;
	return a << 24U | b << 16U | c <<  8U | d <<  0U;
}

static inline uint64_t SwapBytes(uint64_t val)
{
	uint64_t a = val >> 0 & 0xFF;
	uint64_t b = val >> 8 & 0xFF;
	uint64_t c = val >> 16 & 0xFF;
	uint64_t d = val >> 24 & 0xFF;
	uint64_t e = val >> 32 & 0xFF;
	uint64_t f = val >> 40 & 0xFF;
	uint64_t g = val >> 48 & 0xFF;
	uint64_t h = val >> 56 & 0xFF;
	return a << 56U | b << 48U | c << 40U | d << 32U |
	       e << 24U | f << 16U | g <<  8U | h <<  0U;
}

template <class T> T HostToLittle(T val)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return val;
#else
	return SwapBytes(val);
#endif
}

template <class T> T LittleToHost(T val)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return val;
#else
	return SwapBytes(val);
#endif
}

template <class T> T HostToBig(T val)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return SwapBytes(val);
#else
	return val;
#endif
}

template <class T> T BigToHost(T val)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return SwapBytes(val);
#else
	return val;
#endif
}

} // namespace Sortix

#endif
