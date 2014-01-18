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

    stdlib/abort.cpp
    Abnormal process termination.

*******************************************************************************/

#include <sys/stat.h>

#include <calltrace.h>
#include <stdlib.h>

#if defined(__is_sortix_kernel)

#include <sortix/kernel/kernel.h>
#include <sortix/kernel/panic.h>

extern "C" void abort(void)
{
	Sortix::PanicF("abort()");
}

#elif __STDC_HOSTED__

extern "C" void abort(void)
{
	struct stat st;
	if ( getenv("LIBC_DEBUG_CALLTRACE") || stat("/etc/calltrace", &st) == 0 )
		calltrace();
	if ( getenv("LIBC_DEBUG_LOOP") || stat("/etc/calltrace_loop", &st) == 0 )
		while ( true );
	// TODO: Send SIGABRT instead!
	_Exit(128 + 6);
}

#else

extern "C" void abort(void)
{
	while ( true ) { };
	__builtin_unreachable();
}

#endif
