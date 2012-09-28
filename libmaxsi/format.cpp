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

	format.cpp
	Provides printf formatting functions that uses callbacks.

*******************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <string.h>

namespace String {

static int ConvertInt32(int32_t num, char* dest)
{
	int result = 0;
	int32_t copy = num;

	if ( num < 0 )
	{
		*dest++ = '-';
		result++;

		int offset = 0;
		while ( copy < -9 ) { copy /= 10; offset++; }
		result += offset;
		while ( offset >= 0 )
		{
			dest[offset] = '0' - num % 10; num /= 10; offset--;
		}
	}
	else
	{
		int offset = 0;
		while ( copy > 9 ) { copy /= 10; offset++; }
		result += offset;
		while ( offset >= 0 )
		{
			dest[offset] = '0' + num % 10; num /= 10; offset--;
		}
	}

	return result + 1;
}

static int ConvertInt64(int64_t num, char* dest)
{
	int result = 0;
	int64_t copy = num;

	if ( num < 0 )
	{
		*dest++ = '-';
		result++;

		int offset = 0;
		while ( copy < -9 ) { copy /= 10; offset++; }
		result += offset;
		while ( offset >= 0 )
		{
			dest[offset] = '0' - num % 10; num /= 10; offset--;
		}
	}
	else
	{
		int offset = 0;
		while ( copy > 9 ) { copy /= 10; offset++; }
		result += offset;
		while ( offset >= 0 )
		{
			dest[offset] = '0' + num % 10; num /= 10; offset--;
		}
	}

	return result + 1;
}

static int ConvertUInt32(uint32_t num, char* dest)
{
	int result = 0;
	uint32_t copy = num;
	int offset = 0;
	while ( copy > 9 ) { copy /= 10; offset++; }
	result += offset;
	while ( offset >= 0 )
	{
		dest[offset] = '0' + num % 10; num /= 10; offset--;
	}

	return result + 1;
}

static int ConvertUInt64(uint64_t num, char* dest)
{
	int result = 0;
	uint64_t copy = num;
	int offset = 0;
	while ( copy > 9 ) { copy /= 10; offset++; }
	result += offset;
	while ( offset >= 0 )
	{
		dest[offset] = '0' + num % 10; num /= 10; offset--;
	}

	return result + 1;
}

static int ConvertUInt32Hex(uint32_t num, char* dest)
{
	char chars[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	                   'A', 'B', 'C', 'D', 'E', 'F' };
	int result = 0;
	uint32_t copy = num;
	int offset = 0;
	while ( copy > 15 ) { copy /= 16; offset++; }
	result += offset;
	while ( offset >= 0 )
	{
		dest[offset] = chars[num % 16]; num /= 16; offset--;
	}

	return result + 1;
}

static int ConvertUInt64Hex(uint64_t num, char* dest)
{
	char chars[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	                   'A', 'B', 'C', 'D', 'E', 'F' };
	int result = 0;
	uint64_t copy = num;
	int offset = 0;
	while ( copy > 15 ) { copy /= 16; offset++; }
	result += offset;
	while ( offset >= 0 )
	{
		dest[offset] = chars[num % 16]; num /= 16; offset--;
	}

	return result + 1;
}

} // namespace String

#define READY_SIZE 128

#define READY_FLUSH() \
	ready[readylen] = '\0'; \
	if ( 0 < readylen && callback && callback(user, ready, readylen) != readylen ) { return SIZE_MAX; } \
	written += readylen; readylen = 0;

extern "C" size_t vprintf_callback(size_t (*callback)(void*, const char*, size_t),
		                           void* user,
		                           const char* restrict format,
		                           va_list parameters)
{
	size_t written = 0;
	size_t readylen = 0;
	char ready[READY_SIZE + 1];

	while ( *format != '\0' )
	{
		char c = *(format++);

		if ( c != '%' )
		{
			if ( READY_SIZE <= readylen ) { READY_FLUSH(); }
			ready[readylen++] = c;
			continue;
		}

		if ( *format == '%' ) { continue; }

		const unsigned UNSIGNED = 0;
		const unsigned INTEGER = (1<<0);
		const unsigned BIT64 = (1<<1);
		const unsigned HEX = (1<<2);
		const unsigned STRING = 8;
		const unsigned CHARACTER = 9;
	#if defined(__x86_64__)
		const unsigned WORDWIDTH = BIT64;
	#else
		const unsigned WORDWIDTH = 0;
	#endif

		// TODO: Support signed datatypes!

		unsigned type = 0;

		bool scanning = true;
		while ( scanning )
		{
			switch( *(format++) )
			{
				case '3':
				case '2':
					break;
				case '6':
				case '4':
					type |= BIT64;
					break;
				case 'p':
					type = WORDWIDTH | HEX;
					scanning = false;
					break;
				case 't':
					type |= INTEGER;
				case 'z':
				case 'l':
					if ( type & WORDWIDTH ) { type |= BIT64; }
					type |= WORDWIDTH;
					break;
				case 'j':
					type |= BIT64;
					break;
				case 'u':
					type |= UNSIGNED;
					scanning = false;
					break;
				case 'd':
				case 'i':
					type |= INTEGER;
					scanning = false;
					break;
				case 'x':
				case 'X':
					type |= HEX;
					scanning = false;
					break;
				case 's':
					type = STRING;
					scanning = false;
					break;
				case 'c':
					type = CHARACTER;
					scanning = false;
					break;
				default:
					return SIZE_MAX;
			}
		}

		switch ( type )
		{
			case INTEGER:
			{
				if ( READY_SIZE - readylen < 10 ) { READY_FLUSH(); }
				int32_t num = va_arg(parameters, int32_t);
				readylen += String::ConvertInt32(num, ready + readylen);
				break;
			}
			case UNSIGNED:
			{
				if ( READY_SIZE - readylen < 10 ) { READY_FLUSH(); }
				uint32_t num = va_arg(parameters, uint32_t);
				readylen += String::ConvertUInt32(num, ready + readylen);
				break;
			}
			case INTEGER | BIT64:
			{
				if ( READY_SIZE - readylen < 10 ) { READY_FLUSH(); }
				int64_t num = va_arg(parameters, int64_t);
				readylen += String::ConvertInt64(num, ready + readylen);
				break;
			}
			case UNSIGNED | BIT64:
			{
				if ( READY_SIZE - readylen < 20 ) { READY_FLUSH(); }
				uint64_t num = va_arg(parameters, uint64_t);
				readylen += String::ConvertUInt64(num, ready + readylen);
				break;
			}
			case INTEGER | HEX:
			case UNSIGNED | HEX:
			{
				if ( READY_SIZE - readylen < 8 ) { READY_FLUSH(); }
				uint32_t num = va_arg(parameters, uint32_t);
				readylen += String::ConvertUInt32Hex(num, ready + readylen);
				break;
			}
			case INTEGER | BIT64 | HEX:
			case UNSIGNED | BIT64 | HEX:
			{
				if ( READY_SIZE - readylen < 16 ) { READY_FLUSH(); }
				uint64_t num = va_arg(parameters, uint64_t);
				readylen += String::ConvertUInt64Hex(num, ready + readylen);
				break;
			}
			case STRING:
			{
				READY_FLUSH();
				const char* str = va_arg(parameters, const char*);
				size_t len = strlen(str);
				if ( callback && callback(user, str, len) != len ) { return SIZE_MAX; }
				written += len;
				break;
			}
			case CHARACTER:
			{
				int c = va_arg(parameters, int);
				if ( READY_SIZE <= readylen ) { READY_FLUSH(); }
				ready[readylen++] = c;
				break;
			}
		}
	}

	READY_FLUSH();

	return written;
}
