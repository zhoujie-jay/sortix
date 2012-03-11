/*******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011, 2012.

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

	rm.cpp
	Remove files or directories.

*******************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <error.h>

int main(int argc, char* argv[])
{
	if ( argc < 2 ) { printf("usage: %s <file> ...\n", argv[0]); return 0; }

	int result = 0;

	for ( int i = 1; i < argc; i++ )
	{
		if ( unlink(argv[i]) )
		{
			error(0, errno, "cannot remove %s", argv[i]);
			result = 1;
		}
	}

	return result;
}
