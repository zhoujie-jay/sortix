/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2014.

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

    unistd/sfork.c
    Create a new task inheriting some properties from the current task.

*******************************************************************************/

#include <signal.h>
#include <string.h>
#include <unistd.h>

pid_t __sfork(int flags, struct tfork* regs);

pid_t sfork(int flags)
{
	struct tfork regs;
	memset(&regs, 0, sizeof(regs));
	regs.altstack.ss_flags = SS_DISABLE;
	sigprocmask(SIG_SETMASK, NULL, &regs.sigmask);
	regs.altstack.ss_flags = SS_DISABLE;
	return __sfork(flags, &regs);
}
