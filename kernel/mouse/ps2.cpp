/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2015.

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

    mouse/ps2.cpp
    PS2 Mouse.

*******************************************************************************/

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <sortix/poll.h>
#include <sortix/stat.h>

#include <sortix/kernel/ioctx.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/ps2.h>

#include "ps2.h"

namespace Sortix {

static const uint8_t DEVICE_RESET_OK = 0xAA;
static const uint8_t DEVICE_ECHO = 0xEE;
static const uint8_t DEVICE_ACK = 0xFA;
static const uint8_t DEVICE_RESEND = 0xFE;
static const uint8_t DEVICE_ERROR = 0xFF;

static const uint8_t DEVICE_CMD_ENABLE_SCAN = 0xF4;
static const uint8_t DEVICE_CMD_DISABLE_SCAN = 0xF5;
static const uint8_t DEVICE_CMD_IDENTIFY = 0xF2;
static const uint8_t DEVICE_CMD_RESET = 0xFF;

static const size_t DEVICE_RETRIES = 5;

PS2Mouse::PS2Mouse()
{
	this->queue = NULL;
	this->queuelength = 0;
	this->queueoffset = 0;
	this->queueused = 0;
	this->owner = NULL;
	this->ownerptr = NULL;
	this->mlock = KTHREAD_MUTEX_INITIALIZER;
}

PS2Mouse::~PS2Mouse()
{
	delete[] queue;
}

void PS2Mouse::PS2DeviceInitialize(void* send_ctx, bool (*send)(void*, uint8_t),
                                      uint8_t* id, size_t id_size)
{
	if ( sizeof(this->id) < id_size )
		id_size = sizeof(this->id);
	this->send_ctx = send_ctx;
	this->send = send;
	this->state = STATE_INIT;
	this->tries = 0;
	memcpy(this->id, id, id_size);
	this->id_size = id_size;
	PS2DeviceOnByte(DEVICE_RESEND);
}

void PS2Mouse::PS2DeviceOnByte(uint8_t byte)
{
	ScopedLock lock(&mlock);

	if ( state == STATE_INIT )
	{
		state = STATE_ENABLE_SCAN;
		tries = DEVICE_RETRIES;
		byte = DEVICE_RESEND;
	}

	if ( state == STATE_ENABLE_SCAN )
	{
		if ( byte == DEVICE_RESEND && tries-- )
		{
			if ( send(send_ctx, DEVICE_CMD_ENABLE_SCAN) )
				return;
		}
		state = STATE_NORMAL;
		tries = DEVICE_RETRIES;
		if ( byte == DEVICE_ACK )
			return;
		byte = DEVICE_RESEND;
	}

	if ( state == STATE_NORMAL )
	{
		PushByte(byte);
		lock.Reset();
		NotifyOwner();
		return;
	}
}

void PS2Mouse::NotifyOwner()
{
	if ( !owner )
		return;
	owner->OnMouseByte(this, ownerptr);
}

void PS2Mouse::SetOwner(PS2MouseDevice* owner, void* user)
{
	this->owner = owner;
	this->ownerptr = user;
	if ( queueused )
		NotifyOwner();
}

bool PS2Mouse::PushByte(uint8_t byte)
{
	// Check if we need to allocate or resize the circular queue.
	if ( queueused == queuelength )
	{
		size_t newqueuelength = queuelength ? 2 * queuelength : 32UL;
		if ( 1024 < newqueuelength )
			return false;
		uint8_t* newqueue = new uint8_t[newqueuelength];
		if ( !newqueue )
			return false;
		size_t elemsize = sizeof(*queue);
		size_t leadingavai = queuelength - queueoffset;
		size_t leading = leadingavai < queueused ? leadingavai : queueused;
		size_t trailing = queueused - leading;
		if ( queue )
		{
			memcpy(newqueue, queue + queueoffset, leading * elemsize);
			memcpy(newqueue + leading, queue, trailing * elemsize);
			delete[] queue;
		}
		queue = newqueue;
		queuelength = newqueuelength;
		queueoffset = 0;
	}

	queue[(queueoffset + queueused++) % queuelength] = byte;
	return true;
}

uint8_t PS2Mouse::PopByte()
{
	if ( !queueused )
		return 0;
	uint8_t byte = queue[queueoffset];
	queueoffset = (queueoffset + 1) % queuelength;
	queueused--;
	return byte;
}

uint8_t PS2Mouse::Read()
{
	ScopedLock lock(&mlock);
	return PopByte();
}

size_t PS2Mouse::GetPending() const
{
	ScopedLock lock(&mlock);
	return queueused;
}

bool PS2Mouse::HasPending() const
{
	ScopedLock lock(&mlock);
	return queueused;
}

PS2MouseDevice::PS2MouseDevice(dev_t dev, mode_t mode, uid_t owner, gid_t group,
                               PS2Mouse* mouse)
{
	inode_type = INODE_TYPE_TTY;
	this->dev = dev;
	this->ino = (ino_t) this;
	this->type = S_IFCHR;
	this->stat_mode = (mode & S_SETABLE) | this->type;
	this->stat_uid = owner;
	this->stat_gid = group;
	this->mouse = mouse;
	this->devlock = KTHREAD_MUTEX_INITIALIZER;
	this->datacond = KTHREAD_COND_INITIALIZER;
	mouse->SetOwner(this, NULL);
}

PS2MouseDevice::~PS2MouseDevice()
{
	delete mouse;
}

ssize_t PS2MouseDevice::read(ioctx_t* ctx, uint8_t* buf, size_t count)
{
	ScopedLockSignal lock(&devlock);
	if ( !lock.IsAcquired() )
		return errno = EINTR, -1;
	size_t sofar = 0;
	while ( sofar < count )
	{
		if ( !mouse->HasPending() )
		{
			if ( sofar != 0 )
				break;
			do
			{
				if ( !kthread_cond_wait_signal(&datacond, &devlock) )
					return sofar ? sofar : -1;
			} while ( !mouse->HasPending() );
		}
		uint8_t byte = mouse->Read();
		if ( !ctx->copy_to_dest(buf + sofar, &byte, 1) )
			return sofar ? sofar : -1;
		sofar++;
	}
	return (ssize_t) sofar;
}

short PS2MouseDevice::PollEventStatus()
{
	short status = 0;
	if ( mouse->HasPending() )
		status |= POLLIN | POLLRDNORM;
	return status;
}

int PS2MouseDevice::poll(ioctx_t* /*ctx*/, PollNode* node)
{
	ScopedLockSignal lock(&devlock);
	short ret_status = PollEventStatus() & node->events;
	if ( ret_status )
	{
		node->master->revents |= ret_status;
		return 0;
	}
	poll_channel.Register(node);
	return errno = EAGAIN, -1;
}

void PS2MouseDevice::OnMouseByte(PS2Mouse* /*mouse*/, void* /*user*/)
{
	ScopedLockSignal lock(&devlock);
	poll_channel.Signal(PollEventStatus());
	kthread_cond_signal(&datacond);
}

} // namespace Sortix
