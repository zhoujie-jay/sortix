/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012, 2013.

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

    stdlib/on_exit.c
    Hooks that are called upon process exit.

*******************************************************************************/

#include <stdlib.h>

int on_exit(void (*hook)(int, void*), void* param)
{
	struct exit_handler* handler =
		(struct exit_handler*) malloc(sizeof(struct exit_handler));
	if ( !handler )
		return -1;
	handler->hook = hook;
	handler->param = param;
	handler->next = __exit_handler_stack;
	__exit_handler_stack = handler;
	return 0;
}
