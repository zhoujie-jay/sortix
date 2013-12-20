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
#include <sortix/kernel/inode.h>
#include <sortix/kernel/keyboard.h>
#include <sortix/kernel/poll.h>
#include "linebuffer.h"

namespace Sortix {

class LogTerminal : public AbstractInode, public KeyboardOwner
{
public:
	LogTerminal(dev_t dev, mode_t mode, uid_t owner, gid_t group,
	            Keyboard* keyboard, KeyboardLayout* kblayout);
	virtual ~LogTerminal();

public:
	virtual int sync(ioctx_t* ctx);
	virtual ssize_t read(ioctx_t* ctx, uint8_t* buf, size_t count);
	virtual ssize_t write(ioctx_t* ctx, const uint8_t* buf, size_t count);
	virtual int tcgetwinsize(ioctx_t* ctx, struct winsize* ws);
	virtual int tcsetpgrp(ioctx_t* ctx, pid_t pgid);
	virtual pid_t tcgetpgrp(ioctx_t* ctx);
	virtual int settermmode(ioctx_t* ctx, unsigned termmode);
	virtual int gettermmode(ioctx_t* ctx, unsigned* termmode);
	virtual int poll(ioctx_t* ctx, PollNode* node);

public:
	virtual void OnKeystroke(Keyboard* keyboard, void* user);

private:
	void ProcessKeystroke(int kbkey);
	void QueueUnicode(uint32_t unicode);
	void CommitLineBuffer();
	short PollEventStatus();

private:
	PollChannel poll_channel;
	mutable kthread_mutex_t termlock;
	kthread_cond_t datacond;
	size_t numwaiting;
	size_t numeofs;
	Keyboard* keyboard;
	KeyboardLayout* kblayout;
	LineBuffer linebuffer;
	size_t partiallywritten;
	unsigned termmode;
	pid_t foreground_pgid;
	bool control;

};

} // namespace Sortix

#endif
