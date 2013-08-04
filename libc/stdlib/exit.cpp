/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

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

    stdlib/exit.cpp
    Terminates the current process.

*******************************************************************************/

#include <dirent.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern "C" { struct exit_handler* __exit_handler_stack = NULL; }

static pthread_mutex_t exit_lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
static bool currently_exiting = false;

extern "C" void exit(int status)
{
	// It's undefined behavior to call this function more than once: If more
	// than one thread calls the function we'll wait until the process dies.
	pthread_mutex_lock(&exit_lock);

	// It's undefined behavior to call this function more than once: If a
	// cleanup function calls this function we'll self-destruct immediately.
	if ( currently_exiting )
		_Exit(status);
	currently_exiting = true;

	while ( __exit_handler_stack )
	{
		__exit_handler_stack->hook(status, __exit_handler_stack->param);
		__exit_handler_stack = __exit_handler_stack->next;
	}

	dcloseall();
	fcloseall();
	_Exit(status);
}
