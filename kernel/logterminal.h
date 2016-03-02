/*
 * Copyright (c) 2012, 2014, 2015, 2016 Jonas 'Sortie' Termansen.
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
 * logterminal.h
 * A simple terminal that writes to the kernel log.
 */

#ifndef SORTIX_LOGTERMINAL_H
#define SORTIX_LOGTERMINAL_H

#include <wchar.h>

#include <sortix/termios.h>

#include <sortix/kernel/kthread.h>
#include <sortix/kernel/inode.h>
#include <sortix/kernel/keyboard.h>
#include <sortix/kernel/poll.h>

#include "kb/kblayout.h"

#include "linebuffer.h"

namespace Sortix {

class LogTerminal : public AbstractInode, public KeyboardOwner
{
public:
	LogTerminal(dev_t dev, mode_t mode, uid_t owner, gid_t group,
	            Keyboard* keyboard, KeyboardLayoutExecutor* kblayout);
	virtual ~LogTerminal();

public:
	virtual int sync(ioctx_t* ctx);
	virtual ssize_t read(ioctx_t* ctx, uint8_t* buf, size_t count);
	virtual ssize_t write(ioctx_t* ctx, const uint8_t* buf, size_t count);
	virtual int tcgetwincurpos(ioctx_t* ctx, struct wincurpos* wcp);
	virtual int tcgetwinsize(ioctx_t* ctx, struct winsize* ws);
	virtual int tcsetpgrp(ioctx_t* ctx, pid_t pgid);
	virtual pid_t tcgetpgrp(ioctx_t* ctx);
	virtual int settermmode(ioctx_t* ctx, unsigned termmode);
	virtual int gettermmode(ioctx_t* ctx, unsigned* termmode);
	virtual int poll(ioctx_t* ctx, PollNode* node);
	virtual ssize_t tcgetblob(ioctx_t* ctx, const char* name, void* buffer, size_t count);
	virtual ssize_t tcsetblob(ioctx_t* ctx, const char* name, const void* buffer, size_t count);
	virtual int tcdrain(ioctx_t* ctx);
	virtual int tcflow(ioctx_t* ctx, int action);
	virtual int tcflush(ioctx_t* ctx, int queue_selector);
	virtual int tcgetattr(ioctx_t* ctx, struct termios* tio);
	virtual pid_t tcgetsid(ioctx_t* ctx);
	virtual int tcsendbreak(ioctx_t* ctx, int duration);
	virtual int tcsetattr(ioctx_t* ctx, int actions, const struct termios* tio);

public:
	virtual void OnKeystroke(Keyboard* keyboard, void* user);

private:
	void ProcessKeystroke(int kbkey);
	void ProcessString(const char* string);
	void ProcessUnicode(uint32_t unicode);
	void ProcessByte(unsigned char byte, uint32_t control_unicode = 0);
	void CommitLineBuffer();
	short PollEventStatus();
	bool CheckForeground();
	bool RequireForeground(int sig);
	bool CheckHandledByte(tcflag_t lflags, unsigned char key, unsigned char byte);

private:
	PollChannel poll_channel;
	mutable kthread_mutex_t termlock;
	kthread_cond_t datacond;
	mbstate_t read_ps;
	size_t numeofs;
	Keyboard* keyboard;
	KeyboardLayoutExecutor* kblayout;
	LineBuffer linebuffer;
	struct termios tio;
	pid_t foreground_pgid;
	int modifiers;

};

} // namespace Sortix

#endif
