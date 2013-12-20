/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012, 2013.

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

#include <sys/types.h>

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <wchar.h>

#include <sortix/fcntl.h>
#include <sortix/keycodes.h>
#include <sortix/poll.h>
#include <sortix/signal.h>
#include <sortix/stat.h>
#include <sortix/termios.h>
#include <sortix/termmode.h>

#include <sortix/kernel/inode.h>
#include <sortix/kernel/interlock.h>
#include <sortix/kernel/ioctx.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/keyboard.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/poll.h>
#include <sortix/kernel/process.h>
#include <sortix/kernel/refcount.h>
#include <sortix/kernel/scheduler.h>

#include "logterminal.h"

namespace Sortix {

const unsigned SUPPORTED_MODES = TERMMODE_KBKEY
                               | TERMMODE_UNICODE
                               | TERMMODE_SIGNAL
                               | TERMMODE_UTF8
                               | TERMMODE_LINEBUFFER
                               | TERMMODE_ECHO
                               | TERMMODE_NONBLOCK;

LogTerminal::LogTerminal(dev_t dev, mode_t mode, uid_t owner, gid_t group,
                         Keyboard* keyboard, KeyboardLayout* kblayout)
{
	inode_type = INODE_TYPE_TTY;
	this->dev = dev;
	this->ino = (ino_t) this;
	this->type = S_IFCHR;
	this->stat_mode = (mode & S_SETABLE) | this->type;
	this->stat_uid = owner;
	this->stat_gid = group;
	this->keyboard = keyboard;
	this->kblayout = kblayout;
	this->partiallywritten = 0;
	this->control = false;
	this->termmode = TERMMODE_UNICODE
	               | TERMMODE_SIGNAL
	               | TERMMODE_UTF8
	               | TERMMODE_LINEBUFFER
	               | TERMMODE_ECHO;
	this->termlock = KTHREAD_MUTEX_INITIALIZER;
	this->datacond = KTHREAD_COND_INITIALIZER;
	this->numwaiting = 0;
	this->numeofs = 0;
	this->foreground_pgid = 0;
	keyboard->SetOwner(this, NULL);
}

LogTerminal::~LogTerminal()
{
	delete keyboard;
	delete kblayout;
}

int LogTerminal::settermmode(ioctx_t* /*ctx*/, unsigned newtermmode)
{
	if ( newtermmode & ~SUPPORTED_MODES ) { errno = EINVAL; return -1; }
	ScopedLock lock(&termlock);
	unsigned oldtermmode = termmode;
	bool oldutf8 = oldtermmode & TERMMODE_UTF8;
	bool newutf8 = newtermmode & TERMMODE_UTF8;
	if ( oldutf8 != newutf8 )
		partiallywritten = 0;
	termmode = newtermmode;
	if ( (!newtermmode & TERMMODE_LINEBUFFER) )
		CommitLineBuffer();
	partiallywritten = 0;
	return 0;
}

int LogTerminal::gettermmode(ioctx_t* ctx, unsigned* mode)
{
	ScopedLock lock(&termlock);
	if ( !ctx->copy_to_dest(mode, &termmode, sizeof(termmode)) )
		return -1;
	return 0;
}

int LogTerminal::tcgetwincurpos(ioctx_t* ctx, struct wincurpos* wcp)
{
	struct wincurpos retwcp;
	memset(&retwcp, 0, sizeof(retwcp));
	size_t cursor_column, cursor_row;
	Log::GetCursor(&cursor_column, &cursor_row);
	retwcp.wcp_col = cursor_column;
	retwcp.wcp_row = cursor_row;
	if ( !ctx->copy_to_dest(wcp, &retwcp, sizeof(retwcp)) )
		return -1;
	return 0;
}

int LogTerminal::tcgetwinsize(ioctx_t* ctx, struct winsize* ws)
{
	struct winsize retws;
	memset(&retws, 0, sizeof(retws));
	retws.ws_col = Log::Width();
	retws.ws_row = Log::Height();
	if ( !ctx->copy_to_dest(ws, &retws, sizeof(retws)) )
		return -1;
	return 0;
}

int LogTerminal::tcsetpgrp(ioctx_t* /*ctx*/, pid_t pgid)
{
	ScopedLock lock(&termlock);
	Process* process = Process::Get(pgid);
	if ( !pgid || !process )
		return errno = ESRCH, -1;
	kthread_mutex_lock(&process->groupchildlock);
	bool is_process_group = process->group == process;
	kthread_mutex_unlock(&process->groupchildlock);
	if ( !is_process_group )
		return errno = EINVAL, -1;
	foreground_pgid = pgid;
	return 0;
}

pid_t LogTerminal::tcgetpgrp(ioctx_t* /*ctx*/)
{
	ScopedLock lock(&termlock);
	return foreground_pgid;
}

int LogTerminal::sync(ioctx_t* /*ctx*/)
{
	return Log::Sync() ? 0 : -1;
}

void LogTerminal::OnKeystroke(Keyboard* kb, void* /*user*/)
{
	while ( kb->HasPending() )
		ProcessKeystroke(kb->Read());
}

void LogTerminal::ProcessKeystroke(int kbkey)
{
	if ( !kbkey )
		return;

	ScopedLock lock(&termlock);

	if ( kbkey == KBKEY_LCTRL ) { control = true; }
	if ( kbkey == -KBKEY_LCTRL ) { control = false; }
	if ( termmode & TERMMODE_SIGNAL && control && kbkey == KBKEY_C )
	{
		while ( linebuffer.CanBackspace() )
			linebuffer.Backspace();
		if ( foreground_pgid )
		{
			if ( Process* process = Process::Get(foreground_pgid) )
				process->DeliverGroupSignal(SIGINT);
		}
		else // TODO: Backwards compatibility, delete this.
		{
			pid_t pid = Process::HackGetForegroundProcess();
			if ( Process* process = Process::Get(pid) )
				process->DeliverSignal(SIGINT);
		}
		return;
	}
	if ( termmode & TERMMODE_SIGNAL && control && kbkey == KBKEY_D )
	{
		if ( !linebuffer.CanPop() )
		{
			numeofs++;
			if ( numwaiting )
				kthread_cond_broadcast(&datacond);
			poll_channel.Signal(POLLIN | POLLRDNORM);
		}
		return;
	}

	if ( termmode & TERMMODE_LINEBUFFER && control && kbkey == KBKEY_W )
	{
		bool had_non_whitespace = false;
	c_w_delete_more:
		if ( !linebuffer.CanBackspace() ) { return; }
		uint32_t delchar = linebuffer.WouldBackspace();
		bool waskbkey = KBKEY_DECODE(delchar);
		bool wasuni = !waskbkey;
		bool whitespace =
			(wasuni && (delchar == ' ' ||
			            delchar == '\t' ||
			            delchar == '\n')) ||
			(waskbkey && (abs(KBKEY_DECODE(delchar)) == -KBKEY_SPACE ||
			              abs(KBKEY_DECODE(delchar)) == -KBKEY_TAB ||
			              abs(KBKEY_DECODE(delchar)) == -KBKEY_ENTER));
		if ( wasuni && whitespace && had_non_whitespace )
			return;
		if ( wasuni && !whitespace )
			had_non_whitespace = true;
		linebuffer.Backspace();
		if ( (!waskbkey || termmode & TERMMODE_KBKEY) &&
		     (!wasuni || termmode & TERMMODE_UNICODE) &&
		     termmode & TERMMODE_ECHO &&
		     wasuni )
				Log::Print("\b");
		goto c_w_delete_more;
	}

	if ( termmode & TERMMODE_LINEBUFFER && control && kbkey == KBKEY_U )
	{
		while ( linebuffer.CanBackspace() )
		{
			uint32_t delchar = linebuffer.Backspace();
			bool waskbkey = KBKEY_DECODE(delchar);
			bool wasuni = !waskbkey;
			if ( (!waskbkey || termmode & TERMMODE_KBKEY) &&
				 (!wasuni || termmode & TERMMODE_UNICODE) &&
				 termmode & TERMMODE_ECHO &&
				 wasuni )
				Log::Print("\b");
		}
		return;
	}

	if ( termmode & TERMMODE_LINEBUFFER && control && kbkey == KBKEY_L )
	{
		while ( linebuffer.CanBackspace() )
			linebuffer.Backspace();
		QueueUnicode(KBKEY_ENCODE(KBKEY_ENTER));
		QueueUnicode('\n');
		QueueUnicode(KBKEY_ENCODE(-KBKEY_ENTER));
		Log::PrintF("\e[H\e[2J");
		return;
	}

	uint32_t unikbkey = KBKEY_ENCODE(kbkey);
	QueueUnicode(unikbkey);
	uint32_t unicode = kblayout->Translate(kbkey);
	if ( 0 < kbkey )
		QueueUnicode(unicode);
}

void LogTerminal::QueueUnicode(uint32_t unicode)
{
	if ( !unicode )
		return;

	int kbkey = KBKEY_DECODE(unicode);
	int abskbkey = (kbkey < 0) ? -kbkey : kbkey;
	bool wasenter = kbkey == KBKEY_ENTER || unicode == '\n';
	bool kbkeymode = termmode & TERMMODE_KBKEY;
	bool unicodemode = termmode && TERMMODE_UNICODE;
	bool linemode = termmode & TERMMODE_LINEBUFFER;
	bool echomode = termmode & TERMMODE_ECHO;

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
		mbstate_t ps;
		memset(&ps, 0, sizeof(ps));
		char utf8buf[MB_CUR_MAX];
		size_t num_bytes = wcrtomb(utf8buf, (wchar_t) unicode, &ps);
		Log::PrintData(utf8buf, num_bytes);
	}

	bool commit = !linemode || wasenter;
	if ( commit )
		CommitLineBuffer();
}

void LogTerminal::CommitLineBuffer()
{
	linebuffer.Commit();
	if ( numwaiting )
		kthread_cond_broadcast(&datacond);
	poll_channel.Signal(POLLIN | POLLRDNORM);
}

ssize_t LogTerminal::read(ioctx_t* ctx, uint8_t* userbuf, size_t count)
{
	ScopedLockSignal lock(&termlock);
	if ( !lock.IsAcquired() ) { errno = EINTR; return -1; }
	size_t sofar = 0;
	size_t left = count;
	bool blocking = !(termmode & TERMMODE_NONBLOCK);
	while ( left && !linebuffer.CanPop() && blocking && !numeofs )
	{
		if ( ctx->dflags & O_NONBLOCK )
			return errno = EWOULDBLOCK, -1;
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
		if ( !(termmode & TERMMODE_KBKEY) && kbkey ) { ignore = true; }
		if ( !(termmode & TERMMODE_UNICODE) && !kbkey ) { ignore = true; }
		if ( ignore ) { linebuffer.Pop(); continue; }

		uint8_t* buf;
		size_t bufsize;
		uint8_t codepointbuf[4];
		char utf8buf[MB_CUR_MAX];

		if ( termmode & TERMMODE_UTF8 )
		{
			mbstate_t ps;
			memset(&ps, 0, sizeof(ps));
			size_t num_bytes = wcrtomb(utf8buf, (wchar_t) codepoint, &ps);
			if ( num_bytes == (size_t) -1)
			{
				Log::PrintF("Warning: logterminal driver dropping invalid "
				            "codepoint 0x%x\n", codepoint);
				num_bytes = 0;
			}
			buf = (uint8_t*) utf8buf;
			bufsize = num_bytes;
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
		ctx->copy_to_dest(userbuf + sofar, buf, amount);
		partiallywritten = (amount < bufsize) ? partiallywritten + amount : 0;
		left -= amount;
		sofar += amount;
		if ( !partiallywritten ) { linebuffer.Pop(); }
	}

	return sofar;
}

ssize_t LogTerminal::write(ioctx_t* ctx, const uint8_t* buf, size_t count)
{
	// TODO: Add support for ioctx to the kernel log.
	const size_t BUFFER_SIZE = 64UL;
	if ( BUFFER_SIZE < count )
		count = BUFFER_SIZE;
	char buffer[BUFFER_SIZE];
	if ( !ctx->copy_from_src(buffer, buf, count) )
		return -1;
	Log::PrintData(buf, count);
	return count;
}

short LogTerminal::PollEventStatus()
{
	short status = 0;
	if ( linebuffer.CanPop() || numeofs )
		status |= POLLIN | POLLRDNORM;
	if ( true /* can always write */ )
		status |= POLLOUT | POLLWRNORM;
	return status;
}

int LogTerminal::poll(ioctx_t* /*ctx*/, PollNode* node)
{
	ScopedLockSignal lock(&termlock);
	short ret_status = PollEventStatus() & node->events;
	if ( ret_status )
	{
		node->revents |= ret_status;
		return 0;
	}
	poll_channel.Register(node);
	return errno = EAGAIN, -1;
}

} // namespace Sortix
