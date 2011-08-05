/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011.

	This file is part of Sortix.

	Sortix is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License as published by the Free Software
	Foundation, either version 3 of the License, or (at your option) any later
	version.

	Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
	details.

	You should have received a copy of the GNU General Public License along
	with Sortix. If not, see <http://www.gnu.org/licenses/>.

	iprintable.cpp
	A common interface shared by all devices that can be printed text to.

******************************************************************************/

#include "platform.h"
#include "iprintable.h"

namespace Sortix
{
	int UInt8ToString(uint8_t Num, char* Dest)
	{
		uint8_t Copy = Num;
		int Result = 0;

		while ( Copy > 9 ) { Copy /= 10; Result++; }

		int Offset = Result;
		while ( Offset >= 0 )
		{
			Dest[Offset] = '0' + Num % 10; Num /= 10; Offset--;
		}

		return Result + 1;
	}

	int UInt16ToString(uint16_t Num, char* Dest)
	{
		uint16_t Copy = Num;
		int Result = 0;

		while ( Copy > 9 ) { Copy /= 10; Result++; }

		int Offset = Result;
		while ( Offset >= 0 )
		{
			Dest[Offset] = '0' + Num % 10; Num /= 10; Offset--;
		}

		return Result + 1;
	}

	int UInt32ToString(uint32_t Num, char* Dest)
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

	int UInt64ToString(uint64_t Num, char* Dest)
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

	// Missing functions here.

	int UInt32ToString16(uint32_t Num, char* Dest)
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

	int UInt64ToString16(uint64_t Num, char* Dest)
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

	// Missing functions here.

	#define READY_SIZE 128
	#define READY_FLUSH() Ready[AmountReady] = 0; Written += Print(Ready); AmountReady = 0;
	#define F(N) (*(Format+N))

	#define U32 1
	#define U64 2
	#define X32 3
	#define X64 4
	#define STRING 5
	#define CHAR 6

	
	size_t IPrintable::PrintF(const char* Format, ...)
	{
		va_list list;
		va_start(list, Format);
		size_t result = PrintFV(Format, list);
		va_end(list);
		return result;
	}

	size_t IPrintable::PrintFV(const char* Format, va_list Parameters)
	{
		size_t Written = 0;
		char Ready[READY_SIZE + 1];
		int AmountReady = 0;

		while ( *Format != '\0' )
		{
			if ( *Format != '%' )
			{
				Ready[AmountReady] = *Format; AmountReady++; Format++;
			}
			else
			{
				int Type = 0;
				int Len = 1;

				if ( F(1) == 'c' ) { Type = CHAR; Len += 1; } else
				if ( F(1) == 's' ) { Type = STRING; Len += 1; } else
				if ( F(1) == 'u' ) { Type = U32; Len += 1; } else
				if ( F(1) == '3' && F(2) == '2' && F(3) == 'u' ) { Type = U32; Len += 3; } else
				if ( F(1) == '6' && F(2) == '4' && F(3) == 'u' ) { Type = U64; Len += 3; } else
				if ( F(1) == 'x' ) { Type = X32; Len += 1; } else
				if ( F(1) == '3' && F(2) == '2' && F(3) == 'x' ) { Type = X32; Len += 3; } else
				if ( F(1) == '6' && F(2) == '4' && F(3) == 'x' ) { Type = X64; Len += 3; } else
	#ifdef PLATFORM_X86
				if ( F(1) == 'z' ) { Type = U32; Len += 1; } else
				if ( F(1) == 'p' ) { Type = X32; Len += 1; } else
	#elif defined(PLATFORM_X64)
				if ( F(1) == 'z' ) { Type = U64; Len += 1; } else
				if ( F(1) == 'p' ) { Type = X64; Len += 1; } else
	#endif
	//#ifdef PLATFORM_X86_FAMILY
				if ( F(1) == 'j' && F(2) == 'u' ) { Type = U64; Len += 2; } else
				if ( F(1) == 'j' && F(2) == 'x' ) { Type = X64; Len += 2; } else { }
	//#endif

				if ( Type == STRING )
				{
					// TODO: This isn't efficient.
					READY_FLUSH();
					const char* param = va_arg(Parameters, const char*);
					Written += Print(param);
				}
				else if ( Type == CHAR )
				{
					if ( READY_SIZE <= AmountReady ) { READY_FLUSH(); }
					uint32_t param = va_arg(Parameters, uint32_t);
					Ready[AmountReady] = param & 0xFF; AmountReady++;
				}
				else if ( Type == U32 )
				{
					if ( READY_SIZE - AmountReady < 10 ) { READY_FLUSH(); }
					uint32_t Num = va_arg(Parameters, uint32_t);
					AmountReady += UInt32ToString(Num, Ready + AmountReady);
				}
				else if ( Type == U64 )
				{
					if ( READY_SIZE - AmountReady < 20 ) { READY_FLUSH(); }
					uint64_t Num = va_arg(Parameters, uint64_t);
					AmountReady += UInt64ToString(Num, Ready + AmountReady);
				}
				else if ( Type == X32 )
				{
					if ( READY_SIZE - AmountReady < 8 ) { READY_FLUSH(); }
					uint32_t Num = va_arg(Parameters, uint32_t);
					AmountReady += UInt32ToString16(Num, Ready + AmountReady);
				}
				else if ( Type == X64 )
				{
					if ( READY_SIZE - AmountReady < 16 ) { READY_FLUSH(); }
					uint64_t Num = va_arg(Parameters, uint64_t);
					AmountReady += UInt64ToString16(Num, Ready + AmountReady);
				}
				else // Unsupported/unknown format. Just echo!
				{
					Ready[AmountReady] = *Format; AmountReady++; Written++;
				}

				Format += Len;
			}

			if ( READY_SIZE == AmountReady ) { READY_FLUSH(); }
		}

		// Flush our cache.
		if ( AmountReady ) { Ready[AmountReady] = 0; Written += Print(Ready); }

		return Written;
	}
}
