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
		char* Clone(const char* Source);
		char* Combine(size_t NumParameters, ...);
		char* Substring(const char* src, size_t offset, size_t length);
		bool StartsWith(const char* Haystack, const char* Needle);
	}
}

#endif

