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

    endian.h
    Convert byte ordering of integers.

*******************************************************************************/

#ifndef _ENDIAN_H
#define _ENDIAN_H 1

#include <sys/cdefs.h>

#include <__/endian.h>

__BEGIN_DECLS

/* Constans for each kind of known endian. */
#define LITTLE_ENDIAN __LITTLE_ENDIAN
#define PDP_ENDIAN __PDP_ENDIAN
#define BIG_ENDIAN __BIG_ENDIAN

/* Easy access to the current endian. */
#define BYTE_ORDER __BYTE_ORDER
#define FLOAT_WORD_ORDER __FLOAT_WORD_ORDER

/* Easy conversion of 16-bit integers. */
#define htobe16(x) __htobe16(x)
#define htole16(x) __htole16(x)
#define be16toh(x) __be16toh(x)
#define le16toh(x) __le16toh(x)

/* Easy conversion of 32-bit integers. */
#define htobe32(x) __htobe32(x)
#define htole32(x) __htole32(x)
#define be32toh(x) __be32toh(x)
#define le32toh(x) __le32toh(x)

/* Easy conversion of 64-bit integers. */
#define htobe64(x) __htobe64(x)
#define htole64(x) __htole64(x)
#define be64toh(x) __be64toh(x)
#define le64toh(x) __le64toh(x)

__END_DECLS

/* Sortix specific extensions only available in C++. */
#if defined(__cplusplus)

/* Create big-endian versions of the stdint.h exact size data types. */
typedef __big_uint8_t big_uint8_t;
typedef __big_uint16_t big_uint16_t;
typedef __big_uint32_t big_uint32_t;
typedef __big_uint64_t big_uint64_t;

/* Create little-endian versions of the stdint.h exact size data types. */
typedef __little_uint8_t little_uint8_t;
typedef __little_uint16_t little_uint16_t;
typedef __little_uint32_t little_uint32_t;
typedef __little_uint64_t little_uint64_t;

#endif

#endif
