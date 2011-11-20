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

	string.h
	Useful functions for manipulating strings.

******************************************************************************/

#ifndef LIBMAXSI_STRING_H
#define LIBMAXSI_STRING_H

namespace Maxsi
{
	namespace String
	{
		const char ASCII_ESCAPE = 27;		

		size_t Length(const char* String);
		char* Clone(const char* Source);
		char* Combine(size_t NumParameters, ...);
		char* Copy(char* Dest, const char* Src);
		char* Cat(char* Dest, const char* Src);
		int Compare(const char* A, const char* B);
		int CompareN(const char* A, const char* B, size_t MaxLength);
		bool StartsWith(const char* Haystack, const char* Needle);
		int ToInt(const char* str);

		int ConvertUInt8T(uint8_t num, char* dest);
		int ConvertUInt16(uint16_t num, char* dest);
		int ConvertUInt32(uint32_t num, char* dest);
		int ConvertUInt64(uint64_t num, char* dest);
	}
}

#endif

