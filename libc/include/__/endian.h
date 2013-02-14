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

    __/endian.h
    Convert byte ordering of integers.

*******************************************************************************/

#ifndef INCLUDE____ENDIAN_H
#define INCLUDE____ENDIAN_H

#include <features.h>
#include <__/byteswap.h>

__BEGIN_DECLS

/* The compiler provides the type constants. */
#define __LITTLE_ENDIAN __ORDER_LITTLE_ENDIAN__
#define __PDP_ENDIAN __ORDER_PDP_ENDIAN__
#define __BIG_ENDIAN __ORDER_BIG_ENDIAN__

/* The compiler also provides the local endianness as one of the above. */
#define __BYTE_ORDER __BYTE_ORDER__
#define __FLOAT_WORD_ORDER __FLOAT_WORD_ORDER__

/* Declare conversion macros to allow easy endian conversion. */

#if __BYTE_ORDER == __LITTLE_ENDIAN

	#define __htobe16(x) __bswap_16(x)
	#define __htole16(x) (x)
	#define __be16toh(x) __bswap_16(x)
	#define __le16toh(x) (x)

	#define __htobe32(x) __bswap_32(x)
	#define __htole32(x) (x)
	#define __be32toh(x) __bswap_32(x)
	#define __le32toh(x) (x)

	#define __htobe64(x) __bswap_64(x)
	#define __htole64(x) (x)
	#define __be64toh(x) __bswap_64(x)
	#define __le64toh(x) (x)

#elif __BYTE_ORDER == __BIG_ENDIAN

	#define __htobe16(x) (x)
	#define __htole16(x) __bswap_16(x)
	#define __be16toh(x) (x)
	#define __le16toh(x) __bswap_16(x)

	#define __htobe32(x) (x)
	#define __htole32(x) __bswap_32(x)
	#define __be32toh(x) (x)
	#define __le32toh(x) __bswap_32(x)

	#define __htobe64(x) (x)
	#define __htole64(x) __bswap_64(x)
	#define __be64toh(x) (x)
	#define __le64toh(x) __bswap_64(x)

#else

	#error Your processor uses a weird endian, please add support.

#endif

__END_DECLS

#endif
