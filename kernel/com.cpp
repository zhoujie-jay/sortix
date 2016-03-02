/*
 * Copyright (c) 2011, 2012, 2014 Jonas 'Sortie' Termansen.
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
 * com.cpp
 * Handles communication to COM serial ports.
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <sortix/fcntl.h>
#include <sortix/stat.h>

#include <sortix/kernel/descriptor.h>
#include <sortix/kernel/inode.h>
#include <sortix/kernel/interlock.h>
#include <sortix/kernel/interrupt.h>
#include <sortix/kernel/ioctx.h>
#include <sortix/kernel/ioport.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/process.h>
#include <sortix/kernel/refcount.h>
#include <sortix/kernel/thread.h>

#include "com.h"

extern "C" unsigned char nullpage[4096];

namespace Sortix {
namespace COM {

static const uint16_t TXR = 0; // Transmit register
static const uint16_t RXR = 0; // Receive register
static const uint16_t IER = 1; // Interrupt Enable
static const uint16_t IIR = 2; // Interrupt ID
static const uint16_t FCR = 2; // FIFO control
static const uint16_t LCR = 3; // Line control
static const uint16_t MCR = 4; // Modem control
static const uint16_t LSR = 5; // Line Status
static const uint16_t MSR = 6; // Modem Status
static const uint16_t SCR = 7; // Scratch Register
static const uint16_t DLL = 0; // Divisor Latch Low
static const uint16_t DLM = 1; // Divisor latch High

static const uint8_t LCR_DLAB   = 0x80; // Divisor latch access bit
static const uint8_t LCR_SBC    = 0x40; // Set break control
static const uint8_t LCR_SPAR   = 0x20; // Stick parity (?)
static const uint8_t LCR_EPAR   = 0x10; // Even parity select
static const uint8_t LCR_PARITY = 0x08; // Parity Enable
static const uint8_t LCR_STOP   = 0x04; // Stop bits: 0=1 bit, 1=2 bits
static const uint8_t LCR_WLEN5  = 0x00; // Wordlength: 5 bits
static const uint8_t LCR_WLEN6  = 0x01; // Wordlength: 6 bits
static const uint8_t LCR_WLEN7  = 0x02; // Wordlength: 7 bits
static const uint8_t LCR_WLEN8  = 0x03; // Wordlength: 8 bits

static const uint8_t LSR_TEMT  = 0x40; // Transmitter empty
static const uint8_t LSR_THRE  = 0x20; // Transmit-hold-register empty
static const uint8_t LSR_READY = 0x01; // Data received
static const uint8_t LSR_BOTH_EMPTY = LSR_TEMT | LSR_THRE;

static const uint8_t IIR_NO_INTERRUPT = 1 << 0;
static const uint8_t IIR_INTERRUPT_TYPE = 1 << 1 | 1 << 2 | 1 << 3;
static const uint8_t IIR_TIMEOUT = 1 << 2 | 1 << 3;
static const uint8_t IIR_RECV_LINE_STATUS = 1 << 1 | 1 << 2;
static const uint8_t IIR_RECV_DATA = 1 << 2;
static const uint8_t IIR_SENT_DATA = 1 << 1;
static const uint8_t IIR_MODEM_STATUS = 0;

static const uint8_t IER_DATA = 1 << 0;
static const uint8_t IER_SENT = 1 << 1;
static const uint8_t IER_LINE_STATUS = 1 << 2;
static const uint8_t IER_MODEM_STATUS = 1 << 3;
static const uint8_t IER_SLEEP_MODE = 1 << 4;
static const uint8_t IER_LOW_POWER = 1 << 5;

static const unsigned BASE_BAUD = 1843200 / 16;

static const unsigned int UART_8250 = 1;
static const unsigned int UART_16450 = 2;
static const unsigned int UART_16550 = 3;
static const unsigned int UART_16550A = 4;
static const unsigned int UART_16750 = 5;

static const size_t NUM_COM_PORTS = 4;

// Uses various characteristics of the UART chips to determine the hardware.
static unsigned int HardwareProbe(uint16_t port)
{
	// Set the value "0xE7" to the FCR to test the status of the FIFO flags.
	outport8(port + FCR, 0xE7);
	uint8_t iir = inport8(port + IIR);
	if ( iir & (1 << 6) )
	{
		if ( iir & (1 << 7) )
			return iir & (1 << 5) ? UART_16750 : UART_16550A;
		return UART_16550;
	}

	// See if the scratch register returns what we write into it. The 8520
	// doesn't do it. This is technically undefined behavior, but it is useful
	// to detect hardware versions.
	uint16_t any_value = 0x2A;
	outport8(port + SCR, any_value);
	return inport8(port + SCR) == any_value ? UART_16450 : UART_8250;
}

static inline void WaitForEmptyBuffers(uint16_t port)
{
	while ( (inport8(port + LSR) & LSR_BOTH_EMPTY) != LSR_BOTH_EMPTY )
	{
	}
}

static inline bool IsLineReady(uint16_t port)
{
	return inport8(port + LSR) & LSR_READY;
}

static inline bool CanWriteByte(uint16_t port)
{
	return inport8(port + LSR) & LSR_THRE;
}

class DevCOMPort : public AbstractInode
{
public:
	DevCOMPort(dev_t dev, uid_t owner, gid_t group, mode_t mode, uint16_t port);
	virtual ~DevCOMPort();
	virtual int sync(ioctx_t* ctx);
	virtual ssize_t read(ioctx_t* ctx, uint8_t* buf, size_t count);
	virtual ssize_t write(ioctx_t* ctx, const uint8_t* buf, size_t count);

private:
	kthread_mutex_t port_lock;
	uint16_t port;
	uint8_t pending_input_byte;
	bool has_pending_input_byte;

};

DevCOMPort::DevCOMPort(dev_t dev, uid_t owner, gid_t group, mode_t mode,
                       uint16_t port)
{
	inode_type = INODE_TYPE_STREAM;
	this->port = port;
	this->port_lock = KTHREAD_MUTEX_INITIALIZER;
	this->stat_uid = owner;
	this->stat_gid = group;
	this->type = S_IFCHR;
	this->stat_mode = (mode & S_SETABLE) | this->type;
	this->dev = dev;
	this->ino = (ino_t) this;
	this->has_pending_input_byte = false;
}

DevCOMPort::~DevCOMPort()
{
}

int DevCOMPort::sync(ioctx_t* /*ctx*/)
{
	ScopedLock lock(&port_lock);
	WaitForEmptyBuffers(port);
	return 0;
}

ssize_t DevCOMPort::read(ioctx_t* ctx, uint8_t* dest, size_t count)
{
	ScopedLock lock(&port_lock);

	for ( size_t i = 0; i < count; i++ )
	{
		unsigned long attempt = 0;
		while ( !has_pending_input_byte && !IsLineReady(port) )
		{
			attempt++;
			if ( attempt <= 10 )
				continue;
			if ( attempt <= 15 && !(ctx->dflags & O_NONBLOCK) )
			{
				kthread_yield();
				continue;
			}
			if ( i )
				return (ssize_t) i;
			if ( ctx->dflags & O_NONBLOCK )
				return errno = EWOULDBLOCK, -1;
			if ( Signal::IsPending() )
				return errno = EINTR, -1;
			kthread_yield();
		}

		uint8_t value = has_pending_input_byte ?
		                pending_input_byte :
		                inport8(port + RXR);
		if ( !ctx->copy_to_dest(dest + i, &value, sizeof(value)) )
		{
			has_pending_input_byte = true;
			pending_input_byte = value;
			return i ? (ssize_t) i : -1;
		}

		has_pending_input_byte = false;
	}

	return (ssize_t) count;
}

ssize_t DevCOMPort::write(ioctx_t* ctx, const uint8_t* src, size_t count)
{
	ScopedLock lock(&port_lock);

	for ( size_t i = 0; i < count; i++ )
	{
		unsigned long attempt = 0;
		while ( !CanWriteByte(port) )
		{
			attempt++;
			if ( attempt <= 10 )
				continue;
			if ( attempt <= 15 && !(ctx->dflags & O_NONBLOCK) )
			{
				kthread_yield();
				continue;
			}
			if ( i )
				return (ssize_t) i;
			if ( ctx->dflags & O_NONBLOCK )
				return errno = EWOULDBLOCK, -1;
			if ( Signal::IsPending() )
				return errno = EINTR, -1;
		}

		uint8_t value;
		if ( !ctx->copy_from_src(&value, src + i, sizeof(value)) )
			return i ? (ssize_t) i : -1;
		outport8(port + TXR, value);
	}

	return (ssize_t) count;
}

static Ref<DevCOMPort> com_devices[1 + NUM_COM_PORTS];

void Init(const char* devpath, Ref<Descriptor> slashdev)
{
	uint16_t com_ports[1 + NUM_COM_PORTS];
	unsigned int hw_version[1 + NUM_COM_PORTS];

	const uint16_t* bioscom_ports = (const uint16_t*) (nullpage + 0x400);

	for ( size_t i = 1; i <= NUM_COM_PORTS; i++ )
	{
		if ( !(com_ports[i] = bioscom_ports[i-1]) )
			continue;
		hw_version[i] = HardwareProbe(com_ports[i]);
		outport8(com_ports[i] + IER, 0x0);
	}

	(void) hw_version;

	ioctx_t ctx; SetupKernelIOCtx(&ctx);

	for ( size_t i = 1; i <= NUM_COM_PORTS; i++ )
	{
		uint16_t port = com_ports[i];
		if ( !port )
			continue;
		uint8_t interrupts = 0;
		outport8(port + FCR, 0);
		outport8(port + LCR, 0x80);
		outport8(port + DLL, 0xC);
		outport8(port + DLM, 0x0);
		outport8(port + LCR, 0x3); // 8n1
		outport8(port + MCR, 0x3); // DTR + RTS
		outport8(port + IER, interrupts);
	}

	for ( size_t i = 1; i <= NUM_COM_PORTS; i++ )
	{
		if ( !com_ports[i] )
		{
			com_devices[i] = Ref<DevCOMPort>();
			continue;
		}
		com_devices[i] = Ref<DevCOMPort>(new DevCOMPort(slashdev->dev, 0, 0, 0660, com_ports[i]));
		if ( !com_devices[i] )
			PanicF("Unable to allocate device for COM port %zu", i);
		char name[3 + sizeof(size_t) * 3];
		snprintf(name, sizeof(name), "com%zu", i);
		if ( LinkInodeInDir(&ctx, slashdev, name, com_devices[i]) != 0 )
			PanicF("Unable to link %s/%s to COM port driver.", devpath, name);
	}
}

} // namespace COM
} // namespace Sortix
