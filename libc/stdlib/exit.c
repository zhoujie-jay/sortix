/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2014, 2015.

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

    stdlib/exit.c
    Terminates the current process.

*******************************************************************************/

#include <dirent.h>
#include <DIR.h>
#include <FILE.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct exit_handler* __exit_handler_stack = NULL;

static pthread_mutex_t exit_lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
static bool currently_exiting = false;

static FILE* volatile dummy_file = NULL; // volatile due to constant folding bug
weak_alias(dummy_file, __stdin_used);
weak_alias(dummy_file, __stdout_used);

DIR* __first_dir = NULL;
pthread_mutex_t __first_dir_lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
FILE* __first_file = NULL;
pthread_mutex_t __first_file_lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

static void exit_file(FILE* fp)
{
	if ( !fp )
		return;
	flockfile(fp);
	if ( fp->fflush_indirect )
		fp->fflush_indirect(fp);
	if ( fp->close_func )
		fp->close_func(fp->user);
}

void exit(int status)
{
	// It's undefined behavior to call this function more than once: If more
	// than one thread calls the function we'll wait until the process dies.
	pthread_mutex_lock(&exit_lock);

	// It's undefined behavior to call this function more than once: If a
	// cleanup function calls this function we'll self-destruct immediately.
	if ( currently_exiting )
		_exit(status);
	currently_exiting = true;

	while ( __exit_handler_stack )
	{
		__exit_handler_stack->hook(status, __exit_handler_stack->param);
		__exit_handler_stack = __exit_handler_stack->next;
	}

	pthread_mutex_lock(&__first_file_lock);

	exit_file(__stdin_used);
	exit_file(__stdout_used);
	for ( FILE* fp = __first_file; fp; fp = fp->next )
		exit_file(fp);

	_exit(status);
}
