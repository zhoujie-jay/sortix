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

	uname.cpp
	Print system information.

*******************************************************************************/

#include <sys/kernelinfo.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <error.h>

int main(int argc, char* argv[])
{
	size_t VERSIONSIZE = 256UL;
	char version[VERSIONSIZE];
	if ( kernelinfo("version", version, VERSIONSIZE) )
	{
		error(1, errno, "kernelinfo(\"version\")");
	}
	printf("Sortix %zu bit (%s)\n", sizeof(size_t) * 8UL, version);
	return 0;
}

