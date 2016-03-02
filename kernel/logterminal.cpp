/*
 * Copyright (c) 2012, 2013, 2014, 2015, 2016 Jonas 'Sortie' Termansen.
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
 * logterminal.cpp
 * A simple terminal that writes to the kernel log.
 */

#include <sys/types.h>

#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <sortix/fcntl.h>
#include <sortix/keycodes.h>
#include <sortix/poll.h>
#include <sortix/signal.h>
#include <sortix/stat.h>
#include <sortix/termios.h>
#include <sortix/termmode.h>
#include <sortix/winsize.h>

#include <sortix/kernel/inode.h>
#include <sortix/kernel/interlock.h>
#include <sortix/kernel/ioctx.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/keyboard.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/poll.h>
#include <sortix/kernel/process.h>
#include <sortix/kernel/ptable.h>
#include <sortix/kernel/refcount.h>
#include <sortix/kernel/scheduler.h>
#include <sortix/kernel/thread.h>

#include "logterminal.h"

#define CONTROL(x) (((x) - 64) & 127)
#define M_CONTROL(x) (128 + CONTROL(x))

namespace Sortix {

static const unsigned int SUPPORTED_TERMMODES = TERMMODE_KBKEY
                                              | TERMMODE_UNICODE
                                              | TERMMODE_SIGNAL
                                              | TERMMODE_UTF8
                                              | TERMMODE_LINEBUFFER
                                              | TERMMODE_ECHO
                                              | TERMMODE_NONBLOCK
                                              | TERMMODE_TERMIOS
                                              | TERMMODE_DISABLE;

static const int MODIFIER_ALT = 1 << 0;
static const int MODIFIER_LSHIFT = 1 << 1;
static const int MODIFIER_RSHIFT = 1 << 2;
static const int MODIFIER_LCONTROL = 1 << 3;
static const int MODIFIER_RCONTROL = 1 << 4;

static const int SEQUENCE_1IFMOD = 1 << 0;
static const int SEQUENCE_OSHORT = 1 << 1;

struct kbkey_sequence
{
	const char* sequence;
	int kbkey;
	int flags;
};

static const struct kbkey_sequence kbkey_sequences[] =
{
	{ "\e[A", KBKEY_UP, SEQUENCE_1IFMOD },
	{ "\e[B", KBKEY_DOWN, SEQUENCE_1IFMOD},
	{ "\e[C", KBKEY_RIGHT, SEQUENCE_1IFMOD },
	{ "\e[D", KBKEY_LEFT, SEQUENCE_1IFMOD },
	{ "\e[F", KBKEY_END, SEQUENCE_1IFMOD },
	{ "\e[H", KBKEY_HOME, SEQUENCE_1IFMOD },
	{ "\e[2~", KBKEY_INSERT, 0 },
	{ "\e[3~", KBKEY_DELETE, 0 },
	{ "\e[5~", KBKEY_PGUP, 0 },
	{ "\e[6~", KBKEY_PGDOWN, 0 },
	{ "\e[1P", KBKEY_F1, SEQUENCE_OSHORT },
	{ "\e[1Q", KBKEY_F2, SEQUENCE_OSHORT },
	{ "\e[1R", KBKEY_F3, SEQUENCE_OSHORT },
	{ "\e[1S", KBKEY_F4, SEQUENCE_OSHORT },
	{ "\e[15~", KBKEY_F5, 0 },
	{ "\e[17~", KBKEY_F6, 0 },
	{ "\e[18~", KBKEY_F7, 0 },
	{ "\e[19~", KBKEY_F8, 0 },
	{ "\e[20~", KBKEY_F9, 0 },
	{ "\e[21~", KBKEY_F10, 0 },
	{ "\e[23~", KBKEY_F11, 0 },
	{ "\e[24~", KBKEY_F12, 0 },
};

static inline bool IsByteUnescaped(unsigned char byte)
{
	return (32 <= byte && byte != 127) ||
	       byte == '\t' || byte == '\n' || byte == '\r';
}

static inline bool IsUTF8Continuation(unsigned char byte)
{
	return (byte & 0b11000000) == 0b10000000;
}

static inline const struct kbkey_sequence* LookupKeystrokeSequence(int kbkey)
{
	size_t count = sizeof(kbkey_sequences) / sizeof(kbkey_sequences[0]);
	for ( size_t i = 0; i < count; i++ )
		if ( kbkey_sequences[i].kbkey == kbkey )
			return &kbkey_sequences[i];
	return NULL;
}

LogTerminal::LogTerminal(dev_t dev, mode_t mode, uid_t owner, gid_t group,
                         Keyboard* keyboard, KeyboardLayoutExecutor* kblayout)
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
	this->modifiers = 0;
	memset(&tio, 0, sizeof(tio));
	tio.c_iflag = BRKINT | ICRNL | IXANY | IXON;
	tio.c_oflag = OPOST;
	tio.c_cflag = CS8 /*| CREAD*/ | HUPCL; // CREAD unset for boot security.
	tio.c_lflag = ECHO | ECHOE | ECHOK | ICANON | IEXTEN | ISIG;
	tio.c_cc[VEOF] = CONTROL('D');
	tio.c_cc[VEOL] = M_CONTROL('?');
	tio.c_cc[VERASE] = CONTROL('?');
	tio.c_cc[VINTR] = CONTROL('C');
	tio.c_cc[VKILL] = CONTROL('U');
	tio.c_cc[VMIN] = 1;
	tio.c_cc[VQUIT] = CONTROL('\\');
	tio.c_cc[VSTART] = CONTROL('Q');
	tio.c_cc[VSTOP] = CONTROL('S');
	tio.c_cc[VSUSP] = CONTROL('Z');
	tio.c_cc[VTIME] = 0;
	tio.c_cc[VWERASE] = CONTROL('W');
	tio.c_ispeed = B38400;
	tio.c_ospeed = B38400;
	this->termlock = KTHREAD_MUTEX_INITIALIZER;
	this->datacond = KTHREAD_COND_INITIALIZER;
	this->numeofs = 0;
	this->foreground_pgid = 0;
	keyboard->SetOwner(this, NULL);
}

LogTerminal::~LogTerminal()
{
	delete keyboard;
	delete kblayout;
}

int LogTerminal::settermmode(ioctx_t* /*ctx*/, unsigned int termmode)
{
	ScopedLock lock(&termlock);
	if ( !RequireForeground(SIGTTOU) )
		return errno = EINTR, -1;
	if ( termmode & ~SUPPORTED_TERMMODES )
		return errno = EINVAL, -1;
	tcflag_t old_cflag = tio.c_cflag;
	tcflag_t new_cflag = old_cflag;
	tcflag_t old_lflag = tio.c_lflag;
	tcflag_t new_lflag = old_lflag;
	if ( termmode & TERMMODE_KBKEY )
		new_lflag |= ISORTIX_KBKEY;
	else
		new_lflag &= ~ISORTIX_KBKEY;
	if ( !(termmode & TERMMODE_UNICODE) )
		new_lflag |= ISORTIX_CHARS_DISABLE;
	else
		new_lflag &= ~ISORTIX_CHARS_DISABLE;
	if ( termmode & TERMMODE_SIGNAL )
		new_lflag |= ISIG;
	else
		new_lflag &= ~ISIG;
	if ( !(termmode & TERMMODE_UTF8) )
		new_lflag |= ISORTIX_32BIT;
	else
		new_lflag &= ~ISORTIX_32BIT;
	if ( termmode & TERMMODE_LINEBUFFER )
		new_lflag |= ICANON;
	else
		new_lflag &= ~ICANON;
	if ( termmode & TERMMODE_ECHO )
		new_lflag |= ECHO | ECHOE;
	else
		new_lflag &= ~(ECHO | ECHOE);
	if ( termmode & TERMMODE_NONBLOCK )
		new_lflag |= ISORTIX_NONBLOCK;
	else
		new_lflag &= ~ISORTIX_NONBLOCK;
	if ( !(termmode & TERMMODE_TERMIOS) )
		new_lflag |= ISORTIX_TERMMODE;
	else
		new_lflag &= ~ISORTIX_TERMMODE;
	if ( !(termmode & TERMMODE_DISABLE) )
		new_cflag |= CREAD;
	else
		new_cflag &= ~CREAD;
	bool oldnoutf8 = old_lflag & ISORTIX_32BIT;
	bool newnoutf8 = new_lflag & ISORTIX_32BIT;
	if ( oldnoutf8 != newnoutf8 )
		memset(&read_ps, 0, sizeof(read_ps));
	tio.c_cflag = new_cflag;
	tio.c_lflag = new_lflag;
	if ( !(tio.c_lflag & ICANON) )
		CommitLineBuffer();
	return 0;
}

int LogTerminal::gettermmode(ioctx_t* ctx, unsigned int* mode)
{
	ScopedLock lock(&termlock);
	unsigned int termmode = 0;
	if ( tio.c_lflag & ISORTIX_KBKEY )
		termmode |= TERMMODE_KBKEY;
	if ( !(tio.c_lflag & ISORTIX_CHARS_DISABLE) )
		termmode |= TERMMODE_UNICODE;
	if ( tio.c_lflag & ISIG )
		termmode |= TERMMODE_SIGNAL;
	if ( !(tio.c_lflag & ISORTIX_32BIT) )
		termmode |= TERMMODE_UTF8;
	if ( tio.c_lflag & ICANON )
		termmode |= TERMMODE_LINEBUFFER;
	if ( tio.c_lflag & (ECHO | ECHOE) )
		termmode |= TERMMODE_ECHO;
	if ( tio.c_lflag & ISORTIX_NONBLOCK )
		termmode |= TERMMODE_NONBLOCK;
	if ( !(tio.c_lflag & ISORTIX_TERMMODE) )
		termmode |= TERMMODE_TERMIOS;
	if ( !(tio.c_cflag & CREAD) )
		termmode |= TERMMODE_DISABLE;
	if ( !ctx->copy_to_dest(mode, &termmode, sizeof(termmode)) )
		return -1;
	return 0;
}

int LogTerminal::tcgetwincurpos(ioctx_t* ctx, struct wincurpos* wcp)
{
	ScopedLock lock(&termlock);
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
	ScopedLock lock(&termlock);
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
	if ( !RequireForeground(SIGTTOU) )
		return errno = EINTR, -1;
	if ( pgid <= 0 )
		return errno = ESRCH, -1;
	Process* process = CurrentProcess()->GetPTable()->Get(pgid);
	if ( !process )
		return errno = ESRCH, -1;
	kthread_mutex_lock(&process->groupparentlock);
	bool is_process_group = process->group == process;
	kthread_mutex_unlock(&process->groupparentlock);
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
	ScopedLock lock(&termlock);
	return Log::Sync() ? 0 : -1;
}

void LogTerminal::OnKeystroke(Keyboard* kb, void* /*user*/)
{
	ScopedLock lock(&termlock);
	while ( kb->HasPending() )
	{
		int kbkey = kb->Read();
		if ( kbkey == KBKEY_LALT )
			modifiers |= MODIFIER_ALT;
		else if ( kbkey == -KBKEY_LALT )
			modifiers &= ~MODIFIER_ALT;
		else if ( kbkey == KBKEY_LSHIFT )
			modifiers |= MODIFIER_LSHIFT;
		else if ( kbkey == -KBKEY_LSHIFT )
			modifiers &= ~MODIFIER_LSHIFT;
		else if ( kbkey == KBKEY_RSHIFT )
			modifiers |= MODIFIER_RSHIFT;
		else if ( kbkey == -KBKEY_RSHIFT )
			modifiers &= ~MODIFIER_RSHIFT;
		else if ( kbkey == KBKEY_LCTRL )
			modifiers |= MODIFIER_LCONTROL;
		else if ( kbkey == -KBKEY_LCTRL )
			modifiers &= ~MODIFIER_LCONTROL;
		else if ( kbkey == KBKEY_RCTRL )
			modifiers |= MODIFIER_RCONTROL;
		else if ( kbkey == -KBKEY_RCTRL )
			modifiers &= ~MODIFIER_RCONTROL;
		uint32_t unicode = kblayout->Translate(kbkey);
		if ( !(tio.c_cflag & CREAD) )
			continue;
		ProcessKeystroke(kbkey);
		if ( !unicode )
			continue;
		if ( unicode == '\n' )
			unicode = '\r';
		bool control = modifiers & (MODIFIER_LCONTROL | MODIFIER_RCONTROL);
		if ( !(tio.c_cflag & ISORTIX_TERMMODE) && unicode == '\b' )
			unicode = 127;
		if ( control && unicode == L' ' )
			ProcessByte(0, unicode);
		else if ( control && (L'`' <= unicode && unicode <= L'}') )
			ProcessByte(unicode - L'`', unicode);
		else if ( control && (L'@' <= unicode && unicode <= L'_') )
			ProcessByte(unicode - L'@', unicode);
		else if ( control && unicode == L'?' )
			ProcessByte(127, unicode);
		else
			ProcessUnicode(unicode);
	}
}

void LogTerminal::ProcessKeystroke(int kbkey)
{
	if ( tio.c_lflag & ISORTIX_32BIT )
	{
		if ( tio.c_lflag & ISORTIX_KBKEY )
		{
			uint32_t unikbkey = KBKEY_ENCODE(kbkey);
			if ( !linebuffer.Push(unikbkey) )
				return;
			if ( !(tio.c_lflag & ICANON) )
				CommitLineBuffer();
		}
		return;
	}

	if ( kbkey < 0 )
		return;

	const struct kbkey_sequence* seq = LookupKeystrokeSequence(kbkey);
	if ( !seq )
		return;

	const char* str = seq->sequence;
	size_t len = strlen(str);

	int mods = 0;
	if ( modifiers & (MODIFIER_LSHIFT | MODIFIER_RSHIFT) )
		mods |= 1;
	if ( modifiers & MODIFIER_ALT )
		mods |= 2;
	if ( modifiers & (MODIFIER_LCONTROL | MODIFIER_RCONTROL) )
		mods |= 4;

	if ( (seq->flags & SEQUENCE_OSHORT) && mods == 0 )
	{
		ProcessByte('\e');
		ProcessByte('O');
		ProcessByte((unsigned char) str[len-1]);
		return;
	}

	for ( size_t i = 0; i < len - 1; i++ )
		ProcessByte((unsigned char) str[i]);
	if ( seq->flags & SEQUENCE_1IFMOD && mods != 0 )
		ProcessByte('1');
	if ( mods )
	{
		ProcessByte(';');
		ProcessByte('1' + mods);
	}
	ProcessByte(str[len-1]);
}

void LogTerminal::ProcessString(const char* string)
{
	for ( size_t i = 0; string[i]; i++ )
		ProcessByte((unsigned char) string[i]);
}

void LogTerminal::ProcessUnicode(uint32_t unicode)
{
	mbstate_t ps;
	memset(&ps, 0, sizeof(ps));
	char mb[MB_CUR_MAX];
	size_t amount = wcrtomb(mb, unicode, &ps);
	for ( size_t i = 0; amount != (size_t) -1 && i < amount; i++ )
		ProcessByte((unsigned char) mb[i]);
}

bool LogTerminal::CheckHandledByte(tcflag_t lflags,
                                   unsigned char key,
                                   unsigned char byte)
{
	return (tio.c_lflag & lflags) == lflags && key && key == byte;
}

void LogTerminal::ProcessByte(unsigned char byte, uint32_t control_unicode)
{
	if ( byte == '\r' && tio.c_iflag & IGNCR )
		return;
	if ( byte == '\r' && tio.c_iflag & ICRNL )
		byte = '\n';
	else if ( byte == '\n' && tio.c_iflag & INLCR )
		byte = '\r';

	// TODO: tio.c_cc[VEOL]
	// TODO: tio.c_cc[VSTART]
	// TODO: tio.c_cc[VSTOP]
	// TODO: tio.c_cc[VSUSP]

	if ( CheckHandledByte(ISIG, tio.c_cc[VQUIT], byte) )
	{
		while ( linebuffer.CanBackspace() )
			linebuffer.Backspace();
		if ( Process* process = CurrentProcess()->GetPTable()->Get(foreground_pgid) )
			process->DeliverGroupSignal(SIGQUIT);
		return;
	}

	if ( CheckHandledByte(ISIG, tio.c_cc[VINTR], byte) )
	{
		while ( linebuffer.CanBackspace() )
			linebuffer.Backspace();
		if ( Process* process = CurrentProcess()->GetPTable()->Get(foreground_pgid) )
			process->DeliverGroupSignal(SIGINT);
		return;
	}

	if ( CheckHandledByte(ISIG | ICANON, tio.c_cc[VEOF], byte) )
	{
		if ( !linebuffer.CanPop() )
		{
			numeofs++;
			kthread_cond_broadcast(&datacond);
			poll_channel.Signal(POLLIN | POLLRDNORM);
		}
		return;
	}

	if ( CheckHandledByte(ISIG, tio.c_cc[VQUIT], byte) )
	{
		while ( linebuffer.CanBackspace() )
			linebuffer.Backspace();
		if ( Process* process = CurrentProcess()->GetPTable()->Get(foreground_pgid) )
			process->DeliverGroupSignal(SIGQUIT);
		return;
	}

	if ( CheckHandledByte(ICANON, tio.c_cc[VERASE], byte) ||
	     CheckHandledByte(ICANON | ISORTIX_TERMMODE, '\b', byte) )
	{
		while ( linebuffer.CanBackspace() )
		{
			uint32_t delchar = linebuffer.Backspace();
			if ( 256 <= delchar )
				continue;
			if ( IsUTF8Continuation(delchar) )
				continue;
			if ( tio.c_lflag & ECHOE )
			{
				// TODO: Handle tab specially. (Is that even possible without
				//       knowing cursor position?).
				Log::Print("\b \b");
				if ( !IsByteUnescaped(delchar) )
					Log::Print("\b \b");
			}
			break;
		}
		return;
	}

	if ( CheckHandledByte(ICANON, tio.c_cc[VWERASE], byte) )
	{
		bool had_non_whitespace = false;
		while ( linebuffer.CanBackspace() )
		{
			uint32_t delchar = linebuffer.WouldBackspace();
			if ( 256 <= delchar )
				continue;
			if ( IsUTF8Continuation(delchar) )
				continue;
			if ( delchar == L' ' || delchar == L'\t' || delchar == L'\n' )
			{
				if ( had_non_whitespace )
					break;
			}
			else
				had_non_whitespace = true;
			linebuffer.Backspace();
			if ( tio.c_lflag & ECHOE )
			{
				Log::Print("\b \b");
				if ( !IsByteUnescaped(delchar) )
					Log::Print("\b \b");
			}
		}
		return;
	}

	if ( CheckHandledByte(ICANON, tio.c_cc[VKILL], byte) )
	{
		while ( linebuffer.CanBackspace() )
		{
			uint32_t delchar = linebuffer.Backspace();
			if ( 256 <= delchar )
				continue;
			if ( IsUTF8Continuation(delchar) )
				continue;
			if ( tio.c_lflag & ECHOE )
			{
				Log::Print("\b \b");
				if ( !IsByteUnescaped(delchar) )
					Log::Print("\b \b");
			}
		}
		return;
	}

	if ( CheckHandledByte(ICANON | ISORTIX_TERMMODE, CONTROL('L'), byte) )
	{
		while ( linebuffer.CanBackspace() )
			linebuffer.Backspace();
		ProcessUnicode(KBKEY_ENCODE(KBKEY_ENTER));
		ProcessByte('\n');
		ProcessUnicode(KBKEY_ENCODE(-KBKEY_ENTER));
		Log::PrintF("\e[H\e[2J");
		return;
	}

	if ( tio.c_lflag & ISORTIX_CHARS_DISABLE )
		return;

	if ( control_unicode &&
	     !(tio.c_lflag & (ICANON | ISIG)) &&
	     tio.c_lflag & ISORTIX_KBKEY )
	{
		ProcessUnicode(control_unicode);
		return;
	}

	if ( !linebuffer.Push(byte) )
		return;

	if ( tio.c_lflag & ECHO )
	{
		if ( IsByteUnescaped(byte) )
			Log::PrintData(&byte, 1);
		else
			Log::PrintF("^%c", CONTROL(byte));
	}

	if ( !(tio.c_lflag & ICANON) || byte == '\n' )
		CommitLineBuffer();
}

void LogTerminal::CommitLineBuffer()
{
	linebuffer.Commit();
	if ( linebuffer.CanPop() || numeofs )
	{
		kthread_cond_broadcast(&datacond);
		poll_channel.Signal(POLLIN | POLLRDNORM);
	}
}

ssize_t LogTerminal::read(ioctx_t* ctx, uint8_t* userbuf, size_t count)
{
	ScopedLockSignal lock(&termlock);
	if ( !lock.IsAcquired() )
		return errno = EINTR, -1;
	if ( !RequireForeground(SIGTTIN) )
		return errno = EINTR, -1;
	size_t sofar = 0;
	size_t left = count;
	bool nonblocking = tio.c_lflag & ISORTIX_NONBLOCK ||
	                   ctx->dflags & O_NONBLOCK;
	while ( left )
	{
		while ( !(linebuffer.CanPop() || numeofs) )
		{
			if ( sofar )
				return sofar;
			if ( nonblocking )
				return errno = EWOULDBLOCK, -1;
			if ( !kthread_cond_wait_signal(&datacond, &termlock) )
				return errno = EINTR, -1;
			if ( !RequireForeground(SIGTTIN) )
				return errno = EINTR, -1;
		}
		if ( numeofs )
		{
			if ( sofar )
				return sofar;
			return numeofs--, 0;
		}
		uint32_t codepoint = linebuffer.Peek();
		if ( tio.c_lflag & ISORTIX_32BIT )
		{
			if ( left < sizeof(codepoint) )
				return sofar;
			linebuffer.Pop();
			if ( 256 <= codepoint && !(tio.c_lflag & ISORTIX_KBKEY) )
				continue;
			if ( codepoint < 256 && tio.c_lflag & ISORTIX_CHARS_DISABLE )
				continue;
			if ( codepoint < 256 )
			{
				char c = codepoint;
				wchar_t wc;
				size_t amount = mbrtowc(&wc, &c, 1, &read_ps);
				if ( amount == (size_t) -2 )
					continue;
				if ( amount == (size_t) -1 )
				{
					memset(&read_ps, 0, sizeof(read_ps));
					wc = 0xFFFD; /* REPLACEMENT CHARACTER */
				}
				codepoint = wc;
			}
			if ( !ctx->copy_to_dest(userbuf + sofar, &codepoint,
			                        sizeof(codepoint)) )
				return sofar ? sofar : -1;
			left -= sizeof(codepoint);
			sofar += sizeof(codepoint);
			continue;
		}
		if ( 256 <= codepoint )
		{
			linebuffer.Pop();
			continue;
		}
		unsigned char c = codepoint;
		if ( !ctx->copy_to_dest(userbuf + sofar, &c, 1) )
			return sofar ? sofar : -1;
		linebuffer.Pop();
		left -= 1;
		sofar += 1;
		if ( tio.c_lflag & ICANON && c == '\n' )
			break;
	}
	return sofar;
}

ssize_t LogTerminal::write(ioctx_t* ctx, const uint8_t* io_buffer, size_t count)
{
	ScopedLockSignal lock(&termlock);
	if ( tio.c_lflag & TOSTOP && !RequireForeground(SIGTTOU) )
		return errno = EINTR, -1;
	// TODO: Add support for ioctx to the kernel log.
	const size_t BUFFER_SIZE = 64UL;
	char buffer[BUFFER_SIZE];
	size_t sofar = 0;
	while ( sofar < count )
	{
		size_t amount = count - sofar;
		if ( BUFFER_SIZE < amount )
			amount = BUFFER_SIZE;
		if ( !ctx->copy_from_src(buffer, io_buffer + sofar, amount) )
			return -1;
		Log::PrintData(buffer, amount);
		sofar += amount;
		if ( sofar < count )
		{
			kthread_mutex_unlock(&termlock);
			kthread_yield();
			kthread_mutex_lock(&termlock);
			if ( Signal::IsPending() )
				return sofar;
		}
	}
	return (ssize_t) sofar;
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
		node->master->revents |= ret_status;
		return 0;
	}
	poll_channel.Register(node);
	return errno = EAGAIN, -1;
}

ssize_t LogTerminal::tcgetblob(ioctx_t* ctx, const char* name, void* buffer, size_t count)
{
	if ( !name )
	{
		static const char index[] = "kblayout\0";
		size_t index_size = sizeof(index) - 1;
		if ( buffer && count < index_size )
			return errno = ERANGE, -1;
		if ( buffer && !ctx->copy_to_dest(buffer, &index, index_size) )
			return -1;
		return (ssize_t) index_size;
	}
	else if ( !strcmp(name, "kblayout") )
	{
		ScopedLockSignal lock(&termlock);
		const uint8_t* data;
		size_t size;
		if ( !kblayout->Download(&data, &size) )
			return -1;
		if ( buffer && count < size )
			return errno = ERANGE, -1;
		if ( buffer && !ctx->copy_to_dest(buffer, data, size) )
			return -1;
		return (ssize_t) size;
	}
	else
		return errno = ENOENT, -1;
}

ssize_t LogTerminal::tcsetblob(ioctx_t* ctx, const char* name, const void* buffer, size_t count)
{
	if ( !name )
		return errno = EPERM, -1;
	else if ( !strcmp(name, "kblayout") )
	{
		uint8_t* data = new uint8_t[count];
		if ( !data )
			return -1;
		if ( !ctx->copy_from_src(data, buffer, count) )
			return -1;
		ScopedLockSignal lock(&termlock);
		if ( !kblayout->Upload(data, count) )
			return -1;
		delete[] data;
		return (ssize_t) count;
	}
	else
		return errno = ENOENT, -1;
}

int LogTerminal::tcdrain(ioctx_t* /*ctx*/)
{
	ScopedLockSignal lock(&termlock);
	if ( !RequireForeground(SIGTTOU) )
		return errno = EINTR, -1;
	return 0;
}

int LogTerminal::tcflow(ioctx_t* /*ctx*/, int action)
{
	ScopedLockSignal lock(&termlock);
	if ( !RequireForeground(SIGTTOU) )
		return errno = EINTR, -1;
	switch ( action )
	{
	case TCOOFF: break; // TODO: Suspend output.
	case TCOON: break; // TODO: Resume suspended output.
	case TCIOFF: break; // TODO: Transmit STOP character.
	case TCION: break; // TODO: Transmit START character.
	default: return errno = EINVAL -1;
	}
	return 0;
}

int LogTerminal::tcflush(ioctx_t* /*ctx*/, int queue_selector)
{
	if ( queue_selector & ~TCIOFLUSH )
		return errno = EINVAL, -1;
	ScopedLockSignal lock(&termlock);
	if ( !RequireForeground(SIGTTOU) )
		return errno = EINTR, -1;
	if ( queue_selector & TCIFLUSH )
		linebuffer.Flush();
	return 0;
}

int LogTerminal::tcgetattr(ioctx_t* ctx, struct termios* io_tio)
{
	ScopedLockSignal lock(&termlock);
	if ( !ctx->copy_to_dest(io_tio, &tio, sizeof(tio)) )
		return -1;
	return 0;
}

pid_t LogTerminal::tcgetsid(ioctx_t* /*ctx*/)
{
	// TODO: Implement sessions.
	return 1;
}

int LogTerminal::tcsendbreak(ioctx_t* /*ctx*/, int /*duration*/)
{
	ScopedLockSignal lock(&termlock);
	if ( !RequireForeground(SIGTTOU) )
		return errno = EINTR, -1;
	return 0;
}

int LogTerminal::tcsetattr(ioctx_t* ctx, int actions, const struct termios* io_tio)
{
	ScopedLockSignal lock(&termlock);
	if ( !RequireForeground(SIGTTOU) )
		return errno = EINTR, -1;
	switch ( actions )
	{
	case TCSANOW: break;
	case TCSADRAIN: break;
	case TCSAFLUSH: linebuffer.Flush(); break;
	default: return errno = EINVAL, -1;
	}
	if ( !ctx->copy_from_src(&tio, io_tio, sizeof(tio)) )
		return -1;
	// TODO: Potentially take action here if something changed.
	return 0;
}

bool LogTerminal::RequireForeground(int sig)
{
	Thread* thread = CurrentThread();
	Process* process = thread->process;
	ScopedLock group_lock(&process->groupparentlock);
	if ( process->group->pid == foreground_pgid )
		return true;
	if ( sigismember(&thread->signal_mask, sig) )
		return true;
	ScopedLock signal_lock(&process->signal_lock);
	if ( process->signal_actions[sig].sa_handler == SIG_IGN )
		return true;
	signal_lock.Reset();
	group_lock.Reset();
	process->group->DeliverGroupSignal(sig);
	return false;
}

} // namespace Sortix
