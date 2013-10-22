/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2014.

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

    malloc/heap_get_paranoia.cpp
    Returns how paranoid the heap implementation should be.

*******************************************************************************/

#include <malloc.h>
#include <stdlib.h>

extern "C" int heap_get_paranoia(void)
{
#if defined(PARANOIA_DEFAULT) && !__STDC_HOSTED__
	return PARANOIA_DEFAULT;
#elif defined(PARANOIA_DEFAULT) && __STDC_HOSTED__
	static int cached_paranoia = -1;
	if ( cached_paranoia < 0 )
	{
		if ( const char* paranoia_str = getenv("LIBC_MALLOC_PARANOIA") )
			cached_paranoia = atoi(paranoia_str);
		else
			cached_paranoia = PARANOIA_DEFAULT;
	}
	return cached_paranoia;
#else
	return PARANOIA;
#endif
}
