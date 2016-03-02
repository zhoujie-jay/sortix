/*
 * Copyright (c) 2015 Jonas 'Sortie' Termansen.
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
 * mouse/ps2.h
 * PS2 Mouse.
 */

#ifndef SORTIX_MOUSE_PS2_H
#define SORTIX_MOUSE_PS2_H

#include <stddef.h>
#include <stdint.h>

#include <sortix/kernel/inode.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/poll.h>
#include <sortix/kernel/ps2.h>

namespace Sortix {

class PS2Mouse;
class PS2MouseDevice;

class PS2Mouse : public PS2Device
{
public:
	PS2Mouse();
	virtual ~PS2Mouse();
	virtual uint8_t Read();
	virtual size_t GetPending() const;
	virtual bool HasPending() const;
	virtual void SetOwner(PS2MouseDevice* owner, void* user);
	virtual void PS2DeviceInitialize(void* send_ctx, bool (*send)(void*, uint8_t),
                                     uint8_t* id, size_t id_size);
	virtual void PS2DeviceOnByte(uint8_t byte);

private:
	bool PushByte(uint8_t byte);
	uint8_t PopByte();
	void NotifyOwner();

private:
	mutable kthread_mutex_t mlock;
	uint8_t* queue;
	size_t queuelength;
	size_t queueoffset;
	size_t queueused;
	PS2MouseDevice* owner;
	void* ownerptr;
	void* send_ctx;
	bool (*send)(void*, uint8_t);
	enum
	{
		STATE_INIT = 0,
		STATE_ENABLE_SCAN,
		STATE_NORMAL,
	} state;
	size_t tries;
	uint8_t id[2];
	size_t id_size;

};

class PS2MouseDevice : public AbstractInode
{
public:
	PS2MouseDevice(dev_t dev, mode_t mode, uid_t owner, gid_t group,
	               PS2Mouse* mouse);
	virtual ~PS2MouseDevice();

public:
	virtual ssize_t read(ioctx_t* ctx, uint8_t* buf, size_t count);
	virtual int poll(ioctx_t* ctx, PollNode* node);

public:
	virtual void OnMouseByte(PS2Mouse* keyboard, void* user);

private:
	short PollEventStatus();

private:
	PollChannel poll_channel;
	mutable kthread_mutex_t devlock;
	kthread_cond_t datacond;
	PS2Mouse* mouse;

};

} // namespace Sortix

#endif
