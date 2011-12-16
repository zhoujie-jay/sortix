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

	init.cpp
	Initializes the process by setting up the heap, signal handling,
	static memory and other useful things.

******************************************************************************/

#include "platform.h"

namespace Maxsi
{
	namespace Integer
	{
		static bool IsSpace(char c)
		{
			switch ( c )
			{
				case ' ':
				case '\r':
				case '\f':
				case '\n':
				case '\t':
				case '\v':
					return true;
			}
			return false;
		}

		static int Debase(char c)
		{
			if ( '0' <= c && c <= '9' ) { return c - '0'; }
			if ( 'a' <= c && c <= 'z' ) { return 10 + c - 'a'; }
			if ( 'A' <= c && c <= 'Z' ) { return 10 + c - 'A'; }
			return -1;
		}

		template <class INT, bool UNSIGNED> INT ParseInteger(const char* str, char** endptr, int base)
		{
			const char* origstr = str;
			int origbase = base;
			while ( IsSpace(*str) ) { str++; }
			if ( base < 0 || 36 < base ) { if ( endptr ) { *endptr = (char*) str; } return 0; }
			INT result = 0;
			bool negative = false;
			char c = *str;
			if ( !UNSIGNED && c == '-' ) { str++; negative = true; }
			if ( !UNSIGNED && c == '+' ) { str++; negative = false; }
			if ( !base && str[0] == '0' )
			{
				if ( str[1] == 'x' || str[1] == 'X' ) { str += 2; base = 16; }
				else { str++; base = 8; }
			}
			if ( !base ) { base = 10; }
			if ( origbase == 16 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X') ) { str += 2; }
			size_t numconvertedchars = 0;
			while ( (c = *str ) )
			{
				int val = Debase(c);
				if ( val < 0 ) { break; }
				if ( base <= val ) { break; }
				if ( !UNSIGNED && negative ) { val = -val; }
				// TODO: Detect overflow!
				result = result * (INT) base + (INT) val;
				str++;
				numconvertedchars++;
			}
			if ( !numconvertedchars ) { str = origstr; result = 0; }
			if ( endptr ) { *endptr = (char*) str; }
			return result;
		}

		extern "C" long int strtol(const char* str, char** endptr, int base)
		{
			return ParseInteger<long int, false>(str, endptr, base);
		}

		extern "C" long int strtoll(const char* str, char** endptr, int base)
		{
			return ParseInteger<long long int, false>(str, endptr, base);
		}

		extern "C" unsigned long int strtoul(const char* str, char** endptr, int base)
		{
			return ParseInteger<unsigned long int, true>(str, endptr, base);
		}

		extern "C" unsigned long int strtoull(const char* str, char** endptr, int base)
		{
			return ParseInteger<unsigned long long int, true>(str, endptr, base);
		}
	}
}

