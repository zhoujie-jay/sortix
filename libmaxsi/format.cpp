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

	format.cpp
	Provides printf formatting functions that uses callbacks.

******************************************************************************/

#include "platform.h"
#include "string.h"
#include "format.h"

namespace Maxsi
{
	namespace String
	{
		int ConvertUInt32(uint32_t Num, char* Dest)
		{
			uint32_t Copy = Num;
			int Result = 0;

			while ( Copy > 9 ) { Copy /= 10; Result++; }

			int Offset = Result;
			while ( Offset >= 0 )
			{
				Dest[Offset] = '0' + Num % 10; Num /= 10; Offset--;
			}

			return Result + 1;
		}

		int ConvertUInt64(uint64_t Num, char* Dest)
		{
			uint64_t Copy = Num;
			int Result = 0;

			while ( Copy > 9 ) { Copy /= 10; Result++; }

			int Offset = Result;
			while ( Offset >= 0 )
			{
				Dest[Offset] = '0' + Num % 10; Num /= 10; Offset--;
			}

			return Result + 1;
		}

		int ConvertUInt3216(uint32_t Num, char* Dest)
		{
			uint32_t Copy = Num;
			int Result = 0;

			while ( Copy > 15 ) { Copy /= 16; Result++; }

			int Offset = Result;
			while ( Offset >= 0 )
			{
				if ( Num % 16 < 10 )
				{
					Dest[Offset] = '0' + Num % 16;
				}
				else
				{
					Dest[Offset] = 'A' + (Num % 16) - 10;
				}
				Num /= 16; Offset--;
			}

			return Result + 1;
		}

		int ConvertUInt6416(uint64_t Num, char* Dest)
		{
			uint64_t Copy = Num;
			int Result = 0;

			while ( Copy > 15 ) { Copy /= 16; Result++; }

			int Offset = Result;
			while ( Offset >= 0 )
			{
				if ( Num % 16 < 10 )
				{
					Dest[Offset] = '0' + Num % 16;
				}
				else
				{
					Dest[Offset] = 'A' + (Num % 16) - 10;
				}
				Num /= 16; Offset--;
			}

			return Result + 1;
		}
	}

	namespace Format
	{
		#define READY_SIZE 128

		#define READY_FLUSH() \
			ready[readylen] = '\0'; \
			if ( 0 < readylen && callback && callback(user, ready, readylen) != readylen ) { return SIZE_MAX; } \
			written += readylen; readylen = 0;

		size_t Virtual(Callback callback, void* user, const char* format, va_list parameters)
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

				const nat UNSIGNED = 0;
				const nat BIT64 = (1<<0);
				const nat HEX = (1<<1);
				const nat STRING = 4;
				const nat CHARACTER = 5;
			#ifdef PLATFORM_X64
				const nat WORDWIDTH = BIT64;
			#else
				const nat WORDWIDTH = 0;	
			#endif

				// TODO: Support signed datatypes!

				nat type = 0;

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
						case 'z':
						case 'l':
							type |= WORDWIDTH;
							break;
						case 'j':
							type |= BIT64;
							break;
						case 'u':
							type |= UNSIGNED;
							scanning = false;
							break;
						case 'x':
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
					case UNSIGNED:
					{
						if ( READY_SIZE - readylen < 10 ) { READY_FLUSH(); }
						uint32_t num = va_arg(parameters, uint32_t);
						readylen += String::ConvertUInt32(num, ready + readylen);
						break;
					}
					case UNSIGNED | BIT64:
					{
						if ( READY_SIZE - readylen < 20 ) { READY_FLUSH(); }
						uint64_t num = va_arg(parameters, uint64_t);
						readylen += String::ConvertUInt64(num, ready + readylen);
						break;
					}
					case UNSIGNED | HEX:
					{
						if ( READY_SIZE - readylen < 8 ) { READY_FLUSH(); }
						uint32_t num = va_arg(parameters, uint32_t);
						readylen += String::ConvertUInt3216(num, ready + readylen);
						break;
					}
					case UNSIGNED | BIT64 | HEX:
					{
						if ( READY_SIZE - readylen < 16 ) { READY_FLUSH(); }
						uint64_t num = va_arg(parameters, uint64_t);
						readylen += String::ConvertUInt6416(num, ready + readylen);
						break;
					}
					case STRING:
					{
						READY_FLUSH();
						const char* str = va_arg(parameters, const char*);
						size_t len = String::Length(str);
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
	}
}
