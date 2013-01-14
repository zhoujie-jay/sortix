/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

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

    system.cpp
    Execute a shell command.

*******************************************************************************/

#include <sys/wait.h>

#include <stdlib.h>
#include <unistd.h>

extern "C" int system(const char* command)
{
	const int ret_error = command ? -1 : 0;
	const int exit_error = command ? 127 : 0;
	if ( !command )
		command = "exit 1";
	// TODO: Block SIGHCHLD, SIGINT, anda SIGQUIT while waiting.
	pid_t childpid = fork();
	if ( childpid < 0 )
		return ret_error;
	if ( childpid )
	{
		int status;
		if ( waitpid(childpid, &status, 0) < 0 )
			return ret_error;
		return status;
	}
	execlp("sh", "sh", "-c", command, NULL);
	_exit(exit_error);
}
