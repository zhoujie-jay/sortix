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

    stdlib/on_exit.cpp
    Hooks that is called upon process exit.

*******************************************************************************/

#include <stdlib.h>

struct exithandler
{
	void (*hook)(int, void*);
	void* param;
	struct exithandler* next;
}* exit_handler_stack = NULL;

extern "C" int on_exit(void (*hook)(int, void*), void* param)
{
	struct exithandler* handler = (struct exithandler*) malloc(sizeof(struct exithandler));
	if ( !handler ) { return -1; }
	handler->hook = hook;
	handler->param = param;
	handler->next = exit_handler_stack;
	exit_handler_stack = handler;
	return 0;
}

static void atexit_adapter(int /*status*/, void* user)
{
	((void (*)(void)) user)();
}

extern "C" int atexit(void (*hook)(void))
{
	return on_exit(atexit_adapter, (void*) hook);
}

extern "C" void call_exit_handlers(int status)
{
	while ( exit_handler_stack )
	{
		exit_handler_stack->hook(status, exit_handler_stack->param);
		exit_handler_stack = exit_handler_stack->next;
	}
}
