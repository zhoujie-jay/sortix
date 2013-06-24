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

    stdlib/wcstombs.cpp
    Convert a wide-character string to multibyte string.

*******************************************************************************/

#include <stdlib.h>
#include <wchar.h>

extern "C" size_t wcstombs(char* dst, const wchar_t* src, size_t n)
{
	// Reset the secret conversion state variable in wcsrtombs that is used when
	// ps is NULL by successfully converting the empty string. As always, this
	// is not multithread secure. For some reason, the standards don't mandate
	// that the conversion state is reset when wcsrtombs is called with ps=NULL,
	// which arguably is a feature - but this function is supposed to do it.
	const wchar_t* empty_string = L"";
	wcsrtombs(NULL, &empty_string, 0, NULL);
	return wcsrtombs(dst, &src, n, NULL);
}
