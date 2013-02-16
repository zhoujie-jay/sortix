/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

    This program is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
    more details.

    You should have received a copy of the GNU General Public License along with
    this program. If not, see <http://www.gnu.org/licenses/>.

    mv.cpp
    Rename files and directories.

*******************************************************************************/

#include <errno.h>
#include <error.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
	if ( argc != 3 )
	{
		printf("Usage: %s <from> <to>\n", argv[0]);
		return 0;
	}
	if ( rename(argv[1], argv[2]) != 0 )
		error(1, errno, "rename `%s' to `%s'", argv[1], argv[2]);
	return 0;
}
