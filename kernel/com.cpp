/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

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

    com.cpp
    Handles communication to COM serial ports.

*******************************************************************************/

#include <errno.h>

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

namespace Sortix {
namespace COM {

// It appears this code is unable to get interrupts working correctly. Somehow
// we don't get interrupts upon receiving data, at least under VirtualBox. This
// hack changes the code such that it polls occasionally instead. Hopefully this
// won't cause data loss, but I suspect that it will.
// TODO: It appears that this code causes kernel instability, possibly due to
// the broken way blocking system calls are implemented in Sortix.
#define POLL_HACK 1

// Another alternative is to use the polling code in a completely blocking
// manner. While this may give nicer transfer speeds and less data loss, it
// locks up the whole system.
#define POLL_BLOCKING 0

// Yet another alternative is to use POLL_HACK, but return EGAIN and let user-
// space call retry, rather than relying on the broken syscall interstructure.
#define POLL_EAGAIN 1

#if !POLL_EAGAIN && !POLL_HACK
#error The interrupt-based code was broken in the kthread branch.
#error You need to port this to the new thread/interrupt API.
#warning Oh, and fix the above mentioned bugs too.
#endif

const uint16_t TXR = 0; // Transmit register
const uint16_t RXR = 0; // Receive register
const uint16_t IER = 1; // Interrupt Enable
const uint16_t IIR = 2; // Interrupt ID
const uint16_t FCR = 2; // FIFO control
const uint16_t LCR = 3; // Line control
const uint16_t MCR = 4; // Modem control
const uint16_t LSR = 5; // Line Status
const uint16_t MSR = 6; // Modem Status
const uint16_t SCR = 7; // Scratch Register
const uint16_t DLL = 0; // Divisor Latch Low
const uint16_t DLM = 1; // Divisor latch High

const uint8_t LCR_DLAB = 0x80; // Divisor latch access bit
const uint8_t LCR_SBC = 0x40; // Set break control
const uint8_t LCR_SPAR = 0x20; // Stick parity (?)
const uint8_t LCR_EPAR = 0x10; // Even parity select
const uint8_t LCR_PARITY = 0x08; // Parity Enable
const uint8_t LCR_STOP = 0x04; // Stop bits: 0=1 bit, 1=2 bits
const uint8_t LCR_WLEN5 = 0x00; // Wordlength: 5 bits
const uint8_t LCR_WLEN6 = 0x01; // Wordlength: 6 bits
const uint8_t LCR_WLEN7 = 0x02; // Wordlength: 7 bits
const uint8_t LCR_WLEN8 = 0x03; // Wordlength: 8 bits

const uint8_t LSR_TEMT = 0x40; // Transmitter empty
const uint8_t LSR_THRE = 0x20; // Transmit-hold-register empty
const uint8_t LSR_READY = 0x1; // Data received
const uint8_t LSR_BOTH_EMPTY = LSR_TEMT | LSR_THRE;

const uint8_t IIR_NO_INTERRUPT = (1U<<0U);
const uint8_t IIR_INTERRUPT_TYPE = ((1U<<1U) | (1U<<2U) | (1U<<3U));
const uint8_t IIR_TIMEOUT = ((1U<<2U) | (1U<<3U));
const uint8_t IIR_RECV_LINE_STATUS = ((1U<<1U) | (1U<<2U));
const uint8_t IIR_RECV_DATA = (1U<<2U);
const uint8_t IIR_SENT_DATA = (1U<<1U);
const uint8_t IIR_MODEM_STATUS = 0;

const uint8_t IER_DATA = (1U<<0U);
const uint8_t IER_SENT = (1U<<1U);
const uint8_t IER_LINE_STATUS = (1U<<2U);
const uint8_t IER_MODEM_STATUS = (1U<<3U);
const uint8_t IER_SLEEP_MODE = (1U<<4U);
const uint8_t IER_LOW_POWER = (1U<<5U);

const unsigned BASE_BAUD = 1843200/16;

const unsigned UART8250 = 1;
const unsigned UART16450 = 2;
const unsigned UART16550 = 3;
const unsigned UART16550A = 4;
const unsigned UART16750 = 5;

const size_t NUMCOMPORTS = 4;

// The IO base ports of each COM port.
static uint16_t comports[1+NUMCOMPORTS];

// The results of running HardwareProbe on each COM port.
unsigned hwversion[1+NUMCOMPORTS];

// Uses various characteristics of the UART chips to determine the hardware.
static unsigned HardwareProbe(uint16_t port)
{
	// Set the value "0xE7" to the FCR to test the status of the FIFO flags.
	outport8(port + FCR, 0xE7);
	uint8_t iir = inport8(port + IIR);
	if ( iir & (1U<<6U) )
	{
		if ( iir & (1<<7U) )
		{
			return (iir & (1U<<5U)) ? UART16750 : UART16550A;
		}
		return UART16550;
	}

	// See if the scratch register returns what we write into it. The 8520
	// doesn't do it. This is technically undefined behavior, but it is useful
	// to detect hardware versions.
	uint16_t anyvalue = 0x2A;
	outport8(port + SCR, anyvalue);
	return inport8(port + SCR) == anyvalue ? UART16450 : UART8250;
}

static inline void WaitForEmptyBuffers(uint16_t port)
{
	while ( (inport8(port + LSR) & LSR_BOTH_EMPTY) != LSR_BOTH_EMPTY ) { }
}

static inline bool IsLineReady(uint16_t port)
{
	return inport8(port + LSR) & LSR_READY;
}

static inline bool CanWriteByte(uint16_t port)
{
	return inport8(port + LSR) & LSR_THRE;
}

ssize_t ReadBlocking(uint16_t port, void* buf, size_t size)
{
	if ( SSIZE_MAX < size ) { size = SSIZE_MAX; }
	uint8_t* buffer = (uint8_t*) buf;
	uint8_t interruptsenabled = inport8(port + IER);
	outport8(port + IER, 0);

	for ( size_t i = 0; i < size; i++ )
	{
		while ( !IsLineReady(port) ) { }
		buffer[i] = inport8(port + RXR);
	}

	WaitForEmptyBuffers(port);
	outport8(port + IER, interruptsenabled);
	return size;
}

ssize_t WriteBlocking(uint16_t port, const void* buf, size_t size)
{
	if ( SSIZE_MAX < size ) { size = SSIZE_MAX; }
	const uint8_t* buffer = (const uint8_t*) buf;
	uint8_t interruptsenabled = inport8(port + IER);
	outport8(port + IER, 0);

	for ( size_t i = 0; i < size; i++ )
	{
		while ( !CanWriteByte(port) ) { }
		outport8(port + TXR, buffer[i]);
	}

	WaitForEmptyBuffers(port);
	outport8(port + IER, interruptsenabled);
	return size;
}

void EarlyInit()
{
	// We can fetch COM port information from the BIOS Data Area.
	volatile uint16_t* const bioscomports = (uint16_t* const) 0x0400UL;

	for ( size_t i = 1; i <= NUMCOMPORTS; i++ )
	{
		comports[i] = bioscomports[i-1];
		if ( !comports[i] ) { continue; }
		hwversion[i] = HardwareProbe(comports[i]);
		outport8(comports[i] + IER, 0x0);
	}
}

class DevCOMPort : public AbstractInode
{
public:
	DevCOMPort(dev_t dev, uid_t owner, gid_t group, mode_t mode, uint16_t port);
	virtual ~DevCOMPort();
	virtual int sync(ioctx_t* ctx);
	virtual ssize_t read(ioctx_t* ctx, uint8_t* buf, size_t count);
	virtual ssize_t write(ioctx_t* ctx, const uint8_t* buf, size_t count);

public:
	void OnInterrupt();

private:
	kthread_mutex_t portlock;
	uint16_t port;

};

DevCOMPort::DevCOMPort(dev_t dev, uid_t owner, gid_t group, mode_t mode,
                       uint16_t port)
{
	inode_type = INODE_TYPE_STREAM;
	this->port = port;
	this->portlock = KTHREAD_MUTEX_INITIALIZER;
	this->stat_uid = owner;
	this->stat_gid = group;
	this->type = S_IFCHR;
	this->stat_mode = (mode & S_SETABLE) | this->type;
	this->dev = dev;
	this->ino = (ino_t) this;
}

DevCOMPort::~DevCOMPort()
{
}

int DevCOMPort::sync(ioctx_t* /*ctx*/)
{
	// TODO: Not implemented yet, please wait for all outstanding requests.
	return 0;
}

#if POLL_HACK

ssize_t DevCOMPort::read(ioctx_t* ctx, uint8_t* dest, size_t count)
{
	ScopedLock lock(&portlock);

	for ( size_t i = 0; i < count; i++ )
	{
		int tries = 0;
		while ( !IsLineReady(port) )
		{
			if ( ++tries < 10 )
				continue;
			if ( i )
				return (ssize_t) i;
			if ( ctx->dflags & O_NONBLOCK )
				return errno = EWOULDBLOCK, -1;
			if ( Signal::IsPending() )
				return errno = EINTR, -1;
		}

		uint8_t val = inport8(port + RXR);
		if ( !ctx->copy_to_dest(dest + i, &val, sizeof(val)) )
		{
			// TODO: The byte is lost in this case!
			return i ? (ssize_t) i : -1;
		}
	}

	return (ssize_t) count;
}

ssize_t DevCOMPort::write(ioctx_t* ctx, const uint8_t* src, size_t count)
{
	ScopedLock lock(&portlock);

	for ( size_t i = 0; i < count; i++ )
	{
		int tries = 0;
		while ( !CanWriteByte(port) )
		{
			if ( ++tries < 10 )
				continue;
			if ( i )
				return (ssize_t) i;
			if ( ctx->dflags & O_NONBLOCK )
				return errno = EWOULDBLOCK, -1;
			if ( Signal::IsPending() )
				return errno = EINTR, -1;
		}

		uint8_t val;
		if ( !ctx->copy_from_src(&val, src + i, sizeof(val)) )
			return i ? (ssize_t) i : -1;
		outport8(port + TXR, val);
	}

	return (ssize_t) count;
}

#else

#error Yeah, please port these to the new IO interface.

ssize_t DevCOMPort::Read(byte* dest, size_t count)
{
	if ( !count ) { return 0; }
	if ( SSIZE_MAX < count ) { count = SSIZE_MAX; }
#if POLL_BLOCKING
	return ReadBlocking(port, dest, 1);
#endif
	uint8_t lsr = inport8(port + LSR);
	if ( !(lsr & LSR_READY) )
	{
		Panic("Can't wait for com data receive event");
		Error::Set(EBLOCKING);
		return -1;
	}

	size_t sofar = 0;
	do
	{
		if ( count <= sofar ) { break; }
		dest[sofar++] = inport8(port + RXR);
	} while ( inport8(port + LSR) & LSR_READY);

	return sofar;
}

ssize_t DevCOMPort::Write(const uint8_t* src, size_t count)
{
	if ( !count ) { return 0; }
	if ( SSIZE_MAX < count ) { count = SSIZE_MAX; };
#if POLL_BLOCKING
	return WriteBlocking(port, src, 1);
#endif
	uint8_t lsr = inport8(port + LSR);
	if ( !(lsr & LSR_THRE) )
	{
		Panic("Can't wait for com data sent event");
		Error::Set(EBLOCKING);
		return -1;
	}

	size_t sofar = 0;
	do
	{
		if ( count <= sofar ) { break; }
		outport8(port + TXR, src[sofar++]);
	} while ( inport8(port + LSR) & LSR_THRE );

	return sofar;
}

#endif

void DevCOMPort::OnInterrupt()
{
#if POLL_HACK || POLL_BLOCKING
	return;
#endif

	uint8_t iir = inport8(port + IIR);
	if ( iir & IIR_NO_INTERRUPT ) { return; }
	uint8_t intrtype = iir & IIR_INTERRUPT_TYPE;
	switch ( intrtype )
	{
	case IIR_TIMEOUT:
		inport8(port + RXR);
		break;
	case IIR_RECV_LINE_STATUS:
		// TODO: Proper error handling!
		inport8(port + LSR);
		break;
	case IIR_RECV_DATA:
		Panic("Can't wait for com data sent event");
		break;
	case IIR_SENT_DATA:
		Panic("Can't wait for com data sent event");
		inport8(port + IIR);
		break;
	case IIR_MODEM_STATUS:
		inport8(port + MSR);
		break;
	}
}

Ref<DevCOMPort> comdevices[1+NUMCOMPORTS];

static void UARTIRQHandler(struct interrupt_context* /*intctx*/, void* /*user*/)
{
	for ( size_t i = 1; i <= NUMCOMPORTS; i++ )
	{
		if ( !comdevices[i] ) { continue; }
		comdevices[i]->OnInterrupt();
	}
}

static struct interrupt_handler irq3_handler;
static struct interrupt_handler irq4_handler;

void Init(const char* devpath, Ref<Descriptor> slashdev)
{
	ioctx_t ctx; SetupKernelIOCtx(&ctx);
	for ( size_t i = 1; i <= NUMCOMPORTS; i++ )
	{
		if ( !comports[i] ) { comdevices[i] = Ref<DevCOMPort>(); continue; }
		comdevices[i] = Ref<DevCOMPort>
			(new DevCOMPort(slashdev->dev, 0, 0, 0660, comports[i]));
		if ( !comdevices[i] )
		{
			PanicF("Unable to allocate device for COM port %zu at 0x%x", i,
			       comports[i]);
		}
		char name[5] = "comN";
		name[3] = '0' + i;
		if ( LinkInodeInDir(&ctx, slashdev, name, comdevices[i]) != 0 )
			PanicF("Unable to link %s/%s to COM port driver.", devpath, name);
	}

	irq3_handler.handler = UARTIRQHandler;
	irq4_handler.handler = UARTIRQHandler;

	Interrupt::RegisterHandler(Interrupt::IRQ3, &irq3_handler);
	Interrupt::RegisterHandler(Interrupt::IRQ4, &irq4_handler);

	// Initialize the ports so we can transfer data.
	for ( size_t i = 1; i <= NUMCOMPORTS; i++ )
	{
		uint16_t port = comports[i];
		if ( !port ) { continue; }
#if POLL_HACK || POLL_BLOCKING
		uint8_t interrupts = 0;
#else
		uint8_t interrupts = IER_DATA
		                   | IER_SENT
		                   | IER_LINE_STATUS
		                   | IER_MODEM_STATUS;
#endif
		outport8(port + FCR, 0);
		outport8(port + LCR, 0x80);
		outport8(port + DLL, 0xC);
		outport8(port + DLM, 0x0);
		outport8(port + LCR, 0x3); // 8n1
		outport8(port + MCR, 0x3); // DTR + RTS
		outport8(port + IER, interrupts);
	}
}

} // namespace COM
} // namespace Sortix
