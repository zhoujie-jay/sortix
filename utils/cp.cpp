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

	cp.cpp
	Copy files and directories.

*******************************************************************************/

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <error.h>
#include <string.h>

const char* basename(const char* path)
{
	size_t len = strlen(path);
	while ( len-- ) { if ( path[len] == '/' ) { return path + len + 1; } }
	return path;
}

int main(int argc, char* argv[])
{
	if ( argc != 3 ) { printf("usage: %s <from> <to>\n", argv[0]); return 0; }

	const char* frompath = argv[1];
	const char* topath = argv[2];
	char tobuffer[256];

	int fromfd = open(frompath, O_RDONLY);
	if ( fromfd < 0 ) { error(1, errno, "%s", frompath); return 1; }

	int tofd = open(topath, O_WRONLY | O_TRUNC | O_CREAT, 0777);
	if ( tofd < 0 )
	{
		if ( errno == EISDIR )
		{
			strcpy(tobuffer, topath);
			if ( tobuffer[strlen(tobuffer)-1] != '/' ) { strcat(tobuffer, "/"); }
			strcat(tobuffer, basename(frompath));
			topath = tobuffer;
			tofd = open(topath, O_WRONLY | O_TRUNC | O_CREAT, 0777);
		}

		if ( tofd < 0 ) { error(1, errno, "%s", topath); return 1; }
	}

	while ( true )
	{
		const size_t BUFFER_SIZE = 4096;
		char buffer[BUFFER_SIZE];
		ssize_t bytesread = read(fromfd, buffer, BUFFER_SIZE);
		if ( bytesread < 0 ) { error(1, errno, "read: %s", frompath); return 1; }
		if ( bytesread == 0 ) { return 0; }
		if ( (ssize_t) writeall(tofd, buffer, bytesread) < bytesread )
		{
			error(1, errno, "write: %s", topath);
			return 1;
		}
	}
}
