/*******************************************************************************

	Copyright(C) Jonas 'Sortie' Termansen 2012.

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

	fpipe.cpp
	Provides pipe(2) through the FILE interface.

*******************************************************************************/

#include <stdio.h>
#include <unistd.h>

extern "C" int fpipe(FILE* pipes[2])
{
	int pipefd[2];
	if ( pipe(pipefd) ) { return -1; }
	pipes[0] = fdopen(pipefd[0], "r");
	if ( !pipes[0] ) { close(pipefd[0]); close(pipefd[1]); return -1; }
	pipes[1] = fdopen(pipefd[1], "w");
	if ( !pipes[1] ) { fclose(pipes[0]); close(pipefd[1]); return -1; }
	return 0;
}
