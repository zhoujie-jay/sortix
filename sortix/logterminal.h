/*******************************************************************************

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

	You should have received a copy of the GNU General Public License along with
	Sortix. If not, see <http://www.gnu.org/licenses/>.

	logterminal.h
	A simple terminal that writes to the kernel log.

*******************************************************************************/

#ifndef SORTIX_LOGTERMINAL_H
#define SORTIX_LOGTERMINAL_H

#include <sortix/kernel/kthread.h>
#ifdef GOT_FAKE_KTHREAD
#include "event.h"
#endif
#include "stream.h"
#include "terminal.h"
#include "keyboard.h"
#include "linebuffer.h"

namespace Sortix
{
	class LogTerminal : public DevTerminal, public KeyboardOwner
	{
	public:
		LogTerminal(Keyboard* keyboard, KeyboardLayout* kblayout);
		virtual ~LogTerminal();

	public:
		virtual ssize_t Read(uint8_t* dest, size_t count);
		virtual ssize_t Write(const uint8_t* src, size_t count);
		virtual bool IsReadable();
		virtual bool IsWritable();

	public:
		virtual bool SetMode(unsigned mode);
		virtual bool SetWidth(unsigned width);
		virtual bool SetHeight(unsigned height);
		virtual unsigned GetMode() const;
		virtual unsigned GetWidth() const;
		virtual unsigned GetHeight() const;

	public:
		virtual void OnKeystroke(Keyboard* keyboard, void* user);

	private:
		void ProcessKeystroke(int kbkey);
		void QueueUnicode(uint32_t unicode);
		void CommitLineBuffer();

	private:
		mutable kthread_mutex_t termlock;
		kthread_cond_t datacond;
		size_t numwaiting;
		size_t numeofs;
		Keyboard* keyboard;
		KeyboardLayout* kblayout;
		LineBuffer linebuffer;
#ifdef GOT_FAKE_KTHREAD
		Event queuecommitevent;
#endif
		size_t partiallywritten;
		unsigned mode;
		bool control;

	};
}

#endif
