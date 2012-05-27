/*******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011.

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

	echo.cpp
	Display a line of text.

*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int main(int argc, char* argv[])
{
	int startfrom = 1;
	bool trailingnewline = true;
	if ( 1 < argc && strcmp(argv[1], "-n") == 0 )
	{
		trailingnewline = false;
		startfrom = 2;
	}

	const char* prefix = "";
	for ( int i = startfrom; i < argc; i++ )
	{
		errno = 0;
		printf("%s%s", prefix, argv[i]);
		if ( errno != 0 )
		{
			perror("<stdout>");
			exit(1);
		}
		prefix = " ";
	}

	if ( trailingnewline ) { printf("\n"); }

	return 0;
}
