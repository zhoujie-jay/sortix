/******************************************************************************

	Copyright(C) Jonas 'Sortie' Termansen 2012.

	This file is part of Sortix.

	Sortix is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License as published by the Free Software
	Foundation, either version 3 of the License, or (at your option) any later
	version.

	Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
	details.

	You should have received a copy of the GNU General Public License along
	with Sortix. If not, see <http://www.gnu.org/licenses/>.

	terminal.cpp
	Provides an interface to terminals for user-space.

******************************************************************************/

#include <sortix/kernel/platform.h>
#include <sortix/termios.h>
#include <libmaxsi/error.h>
#include "syscall.h"
#include "process.h"
#include "terminal.h"

using namespace Maxsi;

namespace Sortix
{
	int SysSetTermMode(int fd, unsigned mode)
	{
		Process* process = CurrentProcess();
		Device* dev = process->descriptors.Get(fd);
		if ( !dev ) { Error::Set(EBADF); return -1; }
		if ( !dev->IsType(Device::TERMINAL) ) { Error::Set(ENOTTY); return -1; }
		DevTerminal* term = (DevTerminal*) dev;
		return term->SetMode(mode) ? 0 : -1;
	}

	int SysGetTermMode(int fd, unsigned* mode)
	{
		Process* process = CurrentProcess();
		Device* dev = process->descriptors.Get(fd);
		if ( !dev ) { Error::Set(EBADF); return -1; }
		if ( !dev->IsType(Device::TERMINAL) ) { Error::Set(ENOTTY); return -1; }
		DevTerminal* term = (DevTerminal*) dev;
		// TODO: Check that mode is a valid user-space pointer.
		*mode = term->GetMode();
		return 0;
	}

	int SysIsATTY(int fd)
	{
		Process* process = CurrentProcess();
		Device* dev = process->descriptors.Get(fd);
		if ( !dev ) { Error::Set(EBADF); return 0; }
		if ( !dev->IsType(Device::TERMINAL) ) {  Error::Set(ENOTTY); return 0; }
		return 1;
	}

	int SysTCGetWinSize(int fd, struct winsize* ws)
	{
		Process* process = CurrentProcess();
		Device* dev = process->descriptors.Get(fd);
		if ( !dev ) { Error::Set(EBADF); return -1; }
		if ( !dev->IsType(Device::TERMINAL) ) { Error::Set(ENOTTY); return -1; }
		DevTerminal* term = (DevTerminal*) dev;
		struct winsize ret;
		ret.ws_col = term->GetWidth();
		ret.ws_row = term->GetHeight();
		ret.ws_xpixel = 0; // Not supported by DevTerminal interface.
		ret.ws_ypixel = 0; // Not supported by DevTerminal interface.
		// TODO: Check that ws is a valid user-space pointer.
		*ws = ret;
		return 0;
	}

	void Terminal::Init()
	{
		Syscall::Register(SYSCALL_SETTERMMODE, (void*) SysSetTermMode);
		Syscall::Register(SYSCALL_GETTERMMODE, (void*) SysGetTermMode);
		Syscall::Register(SYSCALL_ISATTY, (void*) SysIsATTY);
		Syscall::Register(SYSCALL_TCGETWINSIZE, (void*) SysTCGetWinSize);
	}
}

