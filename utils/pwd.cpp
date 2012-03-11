/*******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2012.

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

	pwd.cpp
	Prints the current working directory.

*******************************************************************************/

#include <stdio.h>
#include <unistd.h>

int main(int argc, char* argv[])
{
	const size_t WDSIZE = 4096;
	char wd[WDSIZE];
	if ( !getcwd(wd, WDSIZE) ) { perror("getcwd"); return 1; }
	printf("%s\n", wd);
	return 0;
}

