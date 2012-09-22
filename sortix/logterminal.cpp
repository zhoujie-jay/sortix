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

	logterminal.cpp
	A simple terminal that writes to the kernel log.

*******************************************************************************/

#include <sortix/kernel/platform.h>
#include <sortix/keycodes.h>
#include <sortix/signal.h>
#include <libmaxsi/memory.h>
#include <errno.h>
#include "utf8.h"
#include "keyboard.h"
#include "process.h"
#include "scheduler.h"
#include "terminal.h"
#include "logterminal.h"

using namespace Maxsi;

namespace Sortix
{
	const unsigned SUPPORTED_MODES = TERMMODE_KBKEY
	                               | TERMMODE_UNICODE
	                               | TERMMODE_SIGNAL
	                               | TERMMODE_UTF8
	                               | TERMMODE_LINEBUFFER
	                               | TERMMODE_ECHO
	                               | TERMMODE_NONBLOCK;

	LogTerminal::LogTerminal(Keyboard* keyboard, KeyboardLayout* kblayout)
	{
		this->keyboard = keyboard;
		this->kblayout = kblayout;
		this->partiallywritten = 0;
		this->control = false;
		this->mode = TERMMODE_UNICODE
		           | TERMMODE_SIGNAL
		           | TERMMODE_UTF8
		           | TERMMODE_LINEBUFFER
		           | TERMMODE_ECHO;
		this->termlock = KTHREAD_MUTEX_INITIALIZER;
		this->datacond = KTHREAD_COND_INITIALIZER;
		this->numwaiting = 0;
		this->numeofs = 0;
		keyboard->SetOwner(this, NULL);
	}

	LogTerminal::~LogTerminal()
	{
		delete keyboard;
		delete kblayout;
	}

	bool LogTerminal::SetMode(unsigned newmode)
	{
		ScopedLock lock(&termlock);
		unsigned oldmode = mode;
		if ( oldmode & ~SUPPORTED_MODES ) { errno = ENOSYS; return false; }
		bool oldutf8 = mode & TERMMODE_UTF8;
		bool newutf8 = newmode & TERMMODE_UTF8;
		if ( oldutf8 ^ newutf8 ) { partiallywritten = 0; }
		mode = newmode;
		if ( !(newmode & TERMMODE_LINEBUFFER) ) { CommitLineBuffer(); }
		partiallywritten = 0;
		return true;
	}

	bool LogTerminal::SetWidth(unsigned width)
	{
		ScopedLock lock(&termlock);
		if ( width != GetWidth() ) { errno = ENOTSUP; return false; }
		return true;
	}

	bool LogTerminal::SetHeight(unsigned height)
	{
		ScopedLock lock(&termlock);
		if ( height != GetHeight() ) { errno = ENOTSUP; return false; }
		return true;
	}

	unsigned LogTerminal::GetMode() const
	{
		ScopedLock lock(&termlock);
		return mode;
	}

	unsigned LogTerminal::GetWidth() const
	{
		return (unsigned) Log::Width();
	}

	unsigned LogTerminal::GetHeight() const
	{
		return (unsigned) Log::Height();
	}

	void LogTerminal::OnKeystroke(Keyboard* kb, void* /*user*/)
	{
		while ( kb->HasPending() )
		{
			ProcessKeystroke(kb->Read());
		}
	}

	void LogTerminal::ProcessKeystroke(int kbkey)
	{
		if ( !kbkey ) { return; }

		ScopedLock lock(&termlock);

		if ( kbkey == KBKEY_LCTRL ) { control = true; }
		if ( kbkey == -KBKEY_LCTRL ) { control = false; }
		if ( mode & TERMMODE_SIGNAL && control && kbkey == KBKEY_C )
		{
			pid_t pid = Process::HackGetForegroundProcess();
			Process* process = Process::Get(pid);
			if ( process )
				process->DeliverSignal(SIGINT);
			return;
		}
		if ( mode & TERMMODE_SIGNAL && control && kbkey == KBKEY_D )
		{
			if ( !linebuffer.CanPop() )
			{
				numeofs++;
#ifdef GOT_ACTUAL_KTHREAD
				if ( numwaiting )
					kthread_cond_broadcast(&datacond);
#else
				queuecommitevent.Signal();
#endif
			}
			return;
		}


		uint32_t unikbkey = KBKEY_ENCODE(kbkey);
		QueueUnicode(unikbkey);
		uint32_t unicode = kblayout->Translate(kbkey);
		if ( 0 < kbkey ) { QueueUnicode(unicode); }
	}

	void LogTerminal::QueueUnicode(uint32_t unicode)
	{
		if ( !unicode ) { return; }

		int kbkey = KBKEY_DECODE(unicode);
		int abskbkey = (kbkey < 0) ? -kbkey : kbkey;
		bool wasenter = kbkey == KBKEY_ENTER || unicode == '\n';
		bool kbkeymode = mode & TERMMODE_KBKEY;
		bool unicodemode = mode && TERMMODE_UNICODE;
		bool linemode = mode & TERMMODE_LINEBUFFER;
		bool echomode = mode & TERMMODE_ECHO;

		if ( linemode && abskbkey == KBKEY_BKSPC ) { return; }
		while ( linemode && unicode == '\b' )
		{
			if ( !linebuffer.CanBackspace() ) { return; }
			uint32_t delchar = linebuffer.Backspace();
			bool waskbkey = KBKEY_DECODE(delchar);
			bool wasuni = !waskbkey;
			if ( waskbkey && !kbkeymode ) { continue; }
			if ( wasuni && !unicodemode ) { continue; }
			if ( !echomode ) { return; }
			if ( wasuni ) { Log::Print("\b"); }
			return;
		}

		if ( !linebuffer.Push(unicode) )
		{
			Log::PrintF("Warning: LogTerminal driver dropping keystroke due "
			            "to insufficient buffer space\n");
			return;
		}

		// TODO: Could it be the right thing to print the unicode character even
		// if it is unprintable (it's an encoded keystroke)?
		if ( !KBKEY_DECODE(unicode) && echomode )
		{
			char utf8buf[6];
			unsigned numbytes = UTF8::Encode(unicode, utf8buf);
			Log::PrintData(utf8buf, numbytes);
		}

		bool commit = !linemode || wasenter;
		if ( commit ) { CommitLineBuffer(); }
	}

	void LogTerminal::CommitLineBuffer()
	{
		linebuffer.Commit();
#ifdef GOT_ACTUAL_KTHREAD
		if ( numwaiting )
			kthread_cond_broadcast(&datacond);
#else
		queuecommitevent.Signal();
#endif
	}

	ssize_t LogTerminal::Read(uint8_t* dest, size_t count)
	{
		ScopedLockSignal lock(&termlock);
		if ( !lock.IsAcquired() ) { errno = EINTR; return -1; }
		size_t sofar = 0;
		size_t left = count;
#ifdef GOT_ACTUAL_KTHREAD
		bool blocking = !(mode & TERMMODE_NONBLOCK);
		while ( left && !linebuffer.CanPop() && blocking && !numeofs )
		{
			numwaiting++;
			bool abort = !kthread_cond_wait_signal(&datacond, &termlock);
			numwaiting--;
			if ( abort ) { errno = EINTR; return -1; }
		}
		if ( left && !linebuffer.CanPop() && !blocking && !numeofs )
		{
			errno = EWOULDBLOCK;
			return -1;
		}
#endif
		if ( numeofs )
		{
			numeofs--;
			return 0;
		}
		while ( left && linebuffer.CanPop() )
		{
			uint32_t codepoint = linebuffer.Peek();
			int kbkey = KBKEY_DECODE(codepoint);

			bool ignore = false;
			if ( !(mode & TERMMODE_KBKEY) && kbkey ) { ignore = true; }
			if ( !(mode & TERMMODE_UNICODE) && !kbkey ) { ignore = true; }
			if ( ignore ) { linebuffer.Pop(); continue; }

			uint8_t* buf;
			size_t bufsize;
			uint8_t codepointbuf[4];
			char utf8buf[6];

			if ( mode & TERMMODE_UTF8 )
			{
				unsigned numbytes = UTF8::Encode(codepoint, utf8buf);
				if ( !numbytes )
				{
					Log::PrintF("Warning: logterminal driver dropping invalid "
					            "codepoint 0x%x\n", codepoint);
				}
				buf = (uint8_t*) utf8buf;
				bufsize = numbytes;
			}
			else
			{
				codepointbuf[0] = (codepoint >> 24U) & 0xFFU;
				codepointbuf[1] = (codepoint >> 16U) & 0xFFU;
				codepointbuf[2] = (codepoint >> 8U) & 0xFFU;
				codepointbuf[3] = (codepoint >> 0U) & 0xFFU;
				buf = (uint8_t*) &codepointbuf;
				bufsize = sizeof(codepointbuf);
				// TODO: Whoops, the above is big-endian and the user programs
				// expect the data to be in the host endian. That's bad. For now
				// just send the data in host endian, but this will break when
				// terminals are accessed over the network.
				buf = (uint8_t*) &codepoint;
			}
			if ( bufsize < partiallywritten ) { partiallywritten = bufsize; }
			buf += partiallywritten;
			bufsize -= partiallywritten;
			if ( sofar && left < bufsize ) { return sofar; }
			size_t amount = left < bufsize ? left : bufsize;
			Memory::Copy(dest + sofar, buf, amount);
			partiallywritten = (amount < bufsize) ? partiallywritten + amount : 0;
			left -= amount;
			sofar += amount;
			if ( !partiallywritten ) { linebuffer.Pop(); }
		}

#ifdef GOT_FAKE_KTHREAD
		// Block if no data were ready.
		if ( !sofar )
		{
			if ( mode & TERMMODE_NONBLOCK ) { errno = EWOULDBLOCK; }
			else { queuecommitevent.Register(); errno = EBLOCKING; }
			return -1;
		}
#endif
		return sofar;
	}

	ssize_t LogTerminal::Write(const uint8_t* src, size_t count)
	{
		Log::PrintData(src, count);
		return count;
	}

	bool LogTerminal::IsReadable()
	{
		return true;
	}

	bool LogTerminal::IsWritable()
	{
		return true;
	}
}
