/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2014.

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

    stdlib/clearenv.cpp
    Clear the environment and set environ to NULL.

*******************************************************************************/

#include <stdlib.h>
#include <unistd.h>

extern "C" int clearenv()
{
	if ( environ == __environ_malloced )
	{
		for ( size_t i = 0; environ[i]; i++ )
			free(environ[i]);
		free(environ);
		__environ_malloced = NULL;
	}
	environ = NULL;
	return 0;
}
