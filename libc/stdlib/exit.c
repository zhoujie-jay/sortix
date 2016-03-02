/*
 * Copyright (c) 2011, 2012, 2014, 2015 Jonas 'Sortie' Termansen.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * stdlib/exit.c
 * Terminates the current process.
 */

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
