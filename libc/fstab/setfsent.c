/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2015.

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

    fstab/setfsent.c
    Open filesystem table.

*******************************************************************************/

#include <fstab.h>
#include <stdio.h>

FILE* __fstab_file = NULL;

int setfsent(void)
{
	if ( __fstab_file )
		rewind(__fstab_file);
	else if ( !(__fstab_file = fopen("/etc/fstab", "r")) )
		return 0;
	return 1;
}
