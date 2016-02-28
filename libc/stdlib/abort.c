/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2014.

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

    stdlib/abort.c
    Abnormal process termination.

*******************************************************************************/

#include <sys/stat.h>
#include <sys/wait.h>

#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef __is_sortix_libk
#include <libk.h>
#endif

void abort(void)
{
#ifdef __is_sortix_libk
	libk_abort();
#else
	sigset_t set_of_sigabrt;
	sigemptyset(&set_of_sigabrt);
	sigaddset(&set_of_sigabrt, SIGABRT);
	sigprocmask(SIG_UNBLOCK, &set_of_sigabrt, NULL);

	raise(SIGABRT);

	int exit_code = WCONSTRUCT(WNATURE_SIGNALED, 128 + SIGABRT, SIGABRT);
	int exit_flags = EXIT_THREAD_PROCESS | EXIT_THREAD_DUMP_CORE;
	exit_thread(exit_code, exit_flags, NULL);

	__builtin_unreachable();
#endif
}
