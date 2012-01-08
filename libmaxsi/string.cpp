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

	string.cpp
	Useful functions for manipulating strings.

******************************************************************************/

#include "platform.h"
#include "string.h"
#include "memory.h"

namespace Maxsi
{
	namespace String
	{
		DUAL_FUNCTION(size_t, strlen, Length, (const char* String))
		{
			size_t Result = 0;

			while ( String[Result] != '\0' )
			{ 
				Result++;
			}

			return Result;
		}

		DUAL_FUNCTION(char*, strcpy, Copy, (char* Dest, const char* Src))
		{
			char* OriginalDest = Dest;

			while ( *Src != '\0' )
			{ 
				*Dest = *Src;
				Dest++; Src++;
			}

			*Dest = '\0';

			return OriginalDest;
		}

		DUAL_FUNCTION(char*, strcat, Cat, (char* Dest, const char* Src))
		{
			char* OriginalDest = Dest;

			while ( *Dest != '\0' ) { Dest++; }

			while ( *Src != '\0' )
			{ 
				*Dest = *Src;
				Dest++; Src++;
			}

			*Dest = '\0';

			return OriginalDest;
		}

		DUAL_FUNCTION(int, strcmp, Compare, (const char* A, const char* B))
		{
			while ( true )
			{
				if ( *A == '\0' && *B == '\0' ) { return 0; }
				if ( *A < *B ) { return -1; }
				if ( *A > *B ) { return 1; }
				A++; B++;
			}
		}

		DUAL_FUNCTION(int, strncmp, CompareN, (const char* A, const char* B, size_t MaxLength))
		{
			while ( MaxLength-- )
			{
				if ( *A == '\0' && *B == '\0' ) { return 0; }
				if ( *A < *B ) { return -1; }
				if ( *A > *B ) { return 1; }
				A++; B++;
			}

			return 0;
		}

		DUAL_FUNCTION(size_t, strspn, Accept, (const char* str, const char* accept))
		{
			size_t acceptlen = 0;
			while ( accept[acceptlen] ) { acceptlen++; }			
			for ( size_t result = 0; true; result++ )
			{
				char c = str[result];
				if ( !c ) { return result; }
				bool matches = false;
				for ( size_t i = 0; i < acceptlen; i++ )
				{
					if ( str[result] != accept[i] ) { continue; }
					matches = true;
					break;
				}
				if ( !matches ) { return result; }
			}
		}

		DUAL_FUNCTION(size_t, strcspn, Reject, (const char* str, const char* reject))
		{
			size_t rejectlen = 0;
			while ( reject[rejectlen] ) { rejectlen++; }			
			for ( size_t result = 0; true; result++ )
			{
				char c = str[result];
				if ( !c ) { return result; }
				bool matches = false;
				for ( size_t i = 0; i < rejectlen; i++ )
				{
					if ( str[result] != reject[i] ) { continue; }
					matches = true;
					break;
				}
				if ( matches ) { return result; }
			}
		}

		DUAL_FUNCTION(char*, strtok_r, TokenizeR, (char* str, const char* delim, char** saveptr))
		{
			if ( !str && !*saveptr ) { return NULL; }
			if ( !str ) { str = *saveptr; }
			str += Accept(str, delim); // Skip leading
			if ( !*str ) { return NULL; }
			size_t amount = Reject(str, delim);
			if ( str[amount] ) { *saveptr = str + amount + 1; }
			else { *saveptr = NULL; }
			str[amount] = '\0';
			return str;
		}

		DUAL_FUNCTION(char*, strtok, TokenizeR, (char* str, const char* delim))
		{
			static char* lasttokensaveptr = NULL;
			return TokenizeR(str, delim, &lasttokensaveptr);
		}

		bool StartsWith(const char* haystack, const char* needle)
		{
			return CompareN(haystack, needle, Length(needle)) == 0;
		}

		char* Clone(const char* Input)
		{
			size_t InputSize = Length(Input);
			char* Result = new char[InputSize + 1];
			if ( Result == NULL ) { return NULL; }
			Memory::Copy(Result, Input, InputSize + 1);
			return Result;
		}

		DUAL_FUNCTION(int, atoi, ToInt, (const char* str))
		{
			bool negative = ( *str == '-' );
			if ( negative ) { str++; }
			int result = 0;
			int c;
			while ( (c = *str++) != 0 )
			{
				if ( c < '0' || '9' < c ) { return result; }
				result = result * 10 + c - '0';
			}
			return (negative) ? -result : result;
		}

#if 0
		char* Combine(size_t NumParameters, ...)
		{
			va_list param_pt;

			va_start(param_pt, NumParameters);
	
			// First calculate the string length.
			size_t ResultLength = 0;
			const char* TMP = 0;

			for ( size_t I = 0; I < NumParameters; I++ )
			{
				TMP = va_arg(param_pt, const char*);

				if ( TMP != NULL ) { ResultLength += strlen(TMP); }
			}

			// Allocate a string with the desired length.
			char* Result = new char[ResultLength+1];

			Result[0] = 0;

			va_end(param_pt);
			va_start(param_pt, NumParameters);

			size_t ResultOffset = 0;

			for ( size_t I = 0; I < NumParameters; I++ )
			{
				TMP = va_arg(param_pt, const char*);

				if ( TMP )
				{
					size_t TMPLength = strlen(TMP);
					strcpy(Result + ResultOffset, TMP);
					ResultOffset += TMPLength;
				}
			}
	
			return Result;
		}
#endif
	}
}
