/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2014.

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

    errno/errno.c
    Value storing a numeric value representing the last occured error.

*******************************************************************************/

#include <errno.h>
#include <stddef.h>

#ifndef __is_sortix_libk

__thread int errno = 0;

#else

int global_errno = 0;
errno_location_func_t errno_location_func = NULL;

int* get_errno_location(void)
{
	if ( errno_location_func )
		return errno_location_func();
	return &global_errno;
}

void set_errno_location_func(errno_location_func_t func)
{
	errno_location_func = func;
}

#endif
