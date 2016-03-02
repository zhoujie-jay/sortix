/*
 * Copyright (c) 2013 Jonas 'Sortie' Termansen.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * __/endian.h
 * Convert byte ordering of integers.
 */

#ifndef INCLUDE____ENDIAN_H
#define INCLUDE____ENDIAN_H

#include <sys/cdefs.h>

#include <__/byteswap.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The compiler provides the type constants. */
#define __LITTLE_ENDIAN __ORDER_LITTLE_ENDIAN__
#define __PDP_ENDIAN __ORDER_PDP_ENDIAN__
#define __BIG_ENDIAN __ORDER_BIG_ENDIAN__

/* The compiler also provides the local endianness as one of the above. */
#define __BYTE_ORDER __BYTE_ORDER__

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

#ifdef __cplusplus
} /* extern "C" */
#endif

/* Sortix specific extensions only available in C++. */
#if defined(__cplusplus)

#include <__/stdint.h>

/* This template allows creating data types that are stored with a specific
   endianness, but can automatically be used as a normal variable. For instance,
   it allows the creation of a big_uint32_t that is an unsigned 32-bit integer
   stored in big endian format. Any assignments to it will automatically be
   converted to big endian and any reads will automatically be converted to the
   host endian. The template even supports volatile objects, so you can create a
   struct with such data types and then use a volatile pointer to such a struct
   and do memory mapped IO easily and correctly. */
template <typename T, int endianness>
class __endian_base
{
private:
	T representation;

	static T swap(const T& arg)
	{
		/* No need to swap if we already have the correct endianness. */
		if ( __BYTE_ORDER == endianness )
			return arg;
		/* Use the optimized macros from byteswap.h for small objects. */
		if ( sizeof(arg) == 1 )
			return arg;
		else if ( sizeof(arg) == 2 )
			return (T) __bswap_16((__uint16_t) arg);
		else if ( sizeof(arg) == 4 )
			return (T) __bswap_32((__uint32_t) arg);
		else if ( sizeof(arg) == 8 )
			return (T) __bswap_64((__uint64_t) arg);
		/* Convert the input as a byte buffer. */
		union { T input; __uint8_t input_buf[sizeof(T)]; };
		union { T output; __uint8_t output_buf[sizeof(T)]; };
		input = arg;
		for ( unsigned long i = 0; i < sizeof(T); i++ )
			output_buf[i] = input_buf[sizeof(T)-(i+1)];
		return output;
	}

public:
	__endian_base() { }
	__endian_base(const T& t) : representation(swap(t)) { }
	operator T() const { return swap(representation); }
	operator T() const volatile { return swap((const T) representation); }

	__endian_base& operator=(const T& rhs)
	{
		representation = swap(rhs);
		return *this;
	}

	__endian_base& operator=(const __endian_base& rhs)
	{
		/* Self-assignment doesn't do any harm here. */
		representation = rhs.representation;
		return *this;
	}

/* TODO: According to the G++ documentation:

         "When using a reference to volatile, G++ does not treat equivalent
          expressions as accesses to volatiles, but instead issues a warning
          that no volatile is accessed."

         This is problematic in our case as we do want to return a volatile
         reference that isn't implicitly accessed, so that stuff such as
         a = b = c works as intended and with volatile semantics.
         The outcommented code does work, but we get a warning when doing a = b,
         and this warning cannot be disabled. For that reason, we revert to the
         unsafe semantics hoping that a = b = c isn't used. */
#if defined(ACCEPTS_VOLATILE_IMPLICIT_ACCESS_WARNING)
	volatile __endian_base& operator=(const T& rhs) volatile
	{
		representation = swap(rhs);
		return *this;
	}

	volatile __endian_base& operator=(const __endian_base& rhs) volatile
	{
		/* Self-assignment does the right thing in this case. */
		representation = rhs.representation;
		return *this;
	}
#else
	__endian_base& operator=(const T& rhs) volatile
	{
		representation = swap(rhs);
		return *(__endian_base*) this;
	}

	__endian_base& operator=(const __endian_base& rhs) volatile
	{
		/* Self-assignment does the right thing in this case. */
		representation = rhs.representation;
		return *(__endian_base*) this;
	}
#endif

	/* TODO: It could be useful to overload ++, --, and other assignment
	         operators here. */

} __attribute__((packed));

/* Create big-endian versions of the stdint.h exact size data types. */
typedef __endian_base<__uint8_t, __BIG_ENDIAN> __big_uint8_t;
typedef __endian_base<__uint16_t, __BIG_ENDIAN> __big_uint16_t;
typedef __endian_base<__uint32_t, __BIG_ENDIAN> __big_uint32_t;
typedef __endian_base<__uint64_t, __BIG_ENDIAN> __big_uint64_t;

/* Create little-endian versions of the stdint.h exact size data types. */
typedef __endian_base<__uint8_t, __LITTLE_ENDIAN> __little_uint8_t;
typedef __endian_base<__uint16_t, __LITTLE_ENDIAN> __little_uint16_t;
typedef __endian_base<__uint32_t, __LITTLE_ENDIAN> __little_uint32_t;
typedef __endian_base<__uint64_t, __LITTLE_ENDIAN> __little_uint64_t;

#endif

#endif
