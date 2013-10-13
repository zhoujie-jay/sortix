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

#include <sortix/kernel/platform.h>
#include <sortix/kernel/kthread.h>
#include <errno.h>
#include "interrupt.h"
#include "stream.h"
#include "syscall.h"
#include "thread.h"
#include "signal.h"
#include "fs/devfs.h"
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
#ifndef GOT_ACTUAL_KTHREAD
#define POLL_EAGAIN 1
#else
#define POLL_EAGAIN 0
#endif

#if !POLL_EAGAIN && !POLL_HACK && defined(GOT_ACTUAL_KTHREAD)
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
	CPU::OutPortB(port + FCR, 0xE7);
	uint8_t iir = CPU::InPortB(port + IIR);
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
	CPU::OutPortB(port + SCR, anyvalue);
	return CPU::InPortB(port + SCR) == anyvalue ? UART16450 : UART8250;
}

static inline void WaitForEmptyBuffers(uint16_t port)
{
	while ( (CPU::InPortB(port + LSR) & LSR_BOTH_EMPTY) != LSR_BOTH_EMPTY ) { }
}

static inline bool IsLineReady(uint16_t port)
{
	return CPU::InPortB(port + LSR) & LSR_READY;
}

static inline bool CanWriteByte(uint16_t port)
{
	return CPU::InPortB(port + LSR) & LSR_THRE;
}

ssize_t ReadBlocking(uint16_t port, void* buf, size_t size)
{
	if ( SSIZE_MAX < size ) { size = SSIZE_MAX; }
	uint8_t* buffer = (uint8_t*) buf;
	uint8_t interruptsenabled = CPU::InPortB(port + IER);
	CPU::OutPortB(port + IER, 0);

	for ( size_t i = 0; i < size; i++ )
	{
		while ( !IsLineReady(port) ) { }
		buffer[i] = CPU::InPortB(port + RXR);
	}

	WaitForEmptyBuffers(port);
	CPU::OutPortB(port + IER, interruptsenabled);
	return size;
}

ssize_t WriteBlocking(uint16_t port, const void* buf, size_t size)
{
	if ( SSIZE_MAX < size ) { size = SSIZE_MAX; }
	const uint8_t* buffer = (const uint8_t*) buf;
	uint8_t interruptsenabled = CPU::InPortB(port + IER);
	CPU::OutPortB(port + IER, 0);

	for ( size_t i = 0; i < size; i++ )
	{
		while ( !CanWriteByte(port) ) { }
		CPU::OutPortB(port + TXR, buffer[i]);
	}

	WaitForEmptyBuffers(port);
	CPU::OutPortB(port + IER, interruptsenabled);
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
		CPU::OutPortB(comports[i] + IER, 0x0);
	}
}

class DevCOMPort : public DevStream
{
public:
	typedef DevStream BaseClass;

public:
	DevCOMPort(uint16_t port);
	virtual ~DevCOMPort();

public:
	virtual ssize_t Read(uint8_t* dest, size_t count);
	virtual ssize_t Write(const uint8_t* src, size_t count);
	virtual bool IsReadable();
	virtual bool IsWritable();

public:
	void OnInterrupt();

private:
	uint16_t port;
	kthread_mutex_t portlock;
#ifdef GOT_FAKE_KTHREAD
	Event dataevent;
	Event sentevent;
#endif

};

DevCOMPort::DevCOMPort(uint16_t port)
{
	this->port = port;
	this->portlock = KTHREAD_MUTEX_INITIALIZER;
}

DevCOMPort::~DevCOMPort()
{
}

bool DevCOMPort::IsReadable() { return true; }
bool DevCOMPort::IsWritable() { return true; }

#if POLL_HACK

const unsigned TRIES = 1000;

ssize_t DevCOMPort::Read(uint8_t* dest, size_t count)
{
	if ( !count ) { return 0; }
	if ( SSIZE_MAX < count ) { count = SSIZE_MAX; }
	ScopedLock lock(&portlock);

#ifdef GOT_ACTUAL_KTHREAD
	while ( !(CPU::InPortB(port + LSR) & LSR_READY) )
		if ( Signal::IsPending() )
		{
			errno = EINTR;
			return -1;
		}
#else
	uint8_t lsr;
	for ( unsigned i = 0; i < TRIES; i++ )
	{
		lsr = CPU::InPortB(port + LSR);
		if ( lsr & LSR_READY ) { break; }
	}

	if ( !(lsr & LSR_READY) )
	{
#if POLL_EAGAIN
		errno = EAGAIN;
#else
		errno = EBLOCKING;
		Syscall::Yield();
#endif
		return -1;
	}
#endif

	size_t sofar = 0;
	do
	{
		if ( count <= sofar ) { break; }
		dest[sofar++] = CPU::InPortB(port + RXR);
	} while ( CPU::InPortB(port + LSR) & LSR_READY);

	return sofar;
}

ssize_t DevCOMPort::Write(const uint8_t* src, size_t count)
{
	if ( !count ) { return 0; }
	if ( SSIZE_MAX < count ) { count = SSIZE_MAX; };

	ScopedLock lock(&portlock);

#ifdef GOT_ACTUAL_KTHREAD
	while ( !(CPU::InPortB(port + LSR) & LSR_THRE) )
		if ( Signal::IsPending() )
		{
			errno = EINTR;
			return -1;
		}
#else
	uint8_t lsr;
	for ( unsigned i = 0; i < TRIES; i++ )
	{
		lsr = CPU::InPortB(port + LSR);
		if ( lsr & LSR_THRE ) { break; }
	}

	if ( !(lsr & LSR_THRE) )
	{
#if POLL_EAGAIN
		errno = EAGAIN;
#else
		errno = EBLOCKING;
		Syscall::Yield();
#endif
		return -1;
	}
#endif

	size_t sofar = 0;
	do
	{
		if ( count <= sofar ) { break; }
		CPU::OutPortB(port + TXR, src[sofar++]);
	} while ( CPU::InPortB(port + LSR) & LSR_THRE );

	return sofar;
}

#else

ssize_t DevCOMPort::Read(uint8_t* dest, size_t count)
{
	if ( !count ) { return 0; }
	if ( SSIZE_MAX < count ) { count = SSIZE_MAX; }
#if POLL_BLOCKING
	return ReadBlocking(port, dest, 1);
#endif
	uint8_t lsr = CPU::InPortB(port + LSR);
	if ( !(lsr & LSR_READY) )
	{
#ifdef GOT_ACTUAL_KTHREAD
		Panic("Can't wait for com data receive event");
#else
		dataevent.Register();
#endif
		errno = EBLOCKING;
		return -1;
	}

	size_t sofar = 0;
	do
	{
		if ( count <= sofar ) { break; }
		dest[sofar++] = CPU::InPortB(port + RXR);
	} while ( CPU::InPortB(port + LSR) & LSR_READY);

	return sofar;
}

ssize_t DevCOMPort::Write(const uint8_t* src, size_t count)
{
	if ( !count ) { return 0; }
	if ( SSIZE_MAX < count ) { count = SSIZE_MAX; };
#if POLL_BLOCKING
	return WriteBlocking(port, src, 1);
#endif
	uint8_t lsr = CPU::InPortB(port + LSR);
	if ( !(lsr & LSR_THRE) )
	{
#ifdef GOT_ACTUAL_KTHREAD
		Panic("Can't wait for com data sent event");
#else
		sentevent.Register();
#endif
		errno = EBLOCKING;
		return -1;
	}

	size_t sofar = 0;
	do
	{
		if ( count <= sofar ) { break; }
		CPU::OutPortB(port + TXR, src[sofar++]);
	} while ( CPU::InPortB(port + LSR) & LSR_THRE );

	return sofar;
}

#endif

void DevCOMPort::OnInterrupt()
{
#if POLL_HACK || POLL_BLOCKING
	return;
#endif

	uint8_t iir = CPU::InPortB(port + IIR);
	if ( iir & IIR_NO_INTERRUPT ) { return; }
	uint8_t intrtype = iir & IIR_INTERRUPT_TYPE;
	switch ( intrtype )
	{
	case IIR_TIMEOUT:
		CPU::InPortB(port + RXR);
		break;
	case IIR_RECV_LINE_STATUS:
		// TODO: Proper error handling!
		CPU::InPortB(port + LSR);
		break;
	case IIR_RECV_DATA:
#ifdef GOT_ACTUAL_KTHREAD
		Panic("Can't wait for com data sent event");
#else
		dataevent.Signal();
#endif
		break;
	case IIR_SENT_DATA:
#ifdef GOT_ACTUAL_KTHREAD
		Panic("Can't wait for com data sent event");
#else
		sentevent.Signal();
#endif
		CPU::InPortB(port + IIR);
		break;
	case IIR_MODEM_STATUS:
		CPU::InPortB(port + MSR);
		break;
	}
}

DevCOMPort* comdevices[1+NUMCOMPORTS];

static void UARTIRQHandler(CPU::InterruptRegisters* /*regs*/, void* /*user*/)
{
	for ( size_t i = 1; i <= NUMCOMPORTS; i++ )
	{
		if ( !comdevices[i] ) { continue; }
		comdevices[i]->OnInterrupt();
	}
}

void Init()
{
	for ( size_t i = 1; i <= NUMCOMPORTS; i++ )
	{
		if ( !comports[i] ) { comdevices[i] = NULL; continue; }
		comdevices[i] = new DevCOMPort(comports[i]);
		if ( !comdevices[i] )
		{
			PanicF("Unable to allocate device for COM port %zu at 0x%x", i,
			       comports[i]);
		}
		char name[5] = "comN";
		name[3] = '0' + i;
		if ( !DeviceFS::RegisterDevice(name, comdevices[i]) )
		{
			PanicF("Unable to register device /dev/%s", name);
		}
	}

	Interrupt::RegisterHandler(Interrupt::IRQ3, UARTIRQHandler, NULL);
	Interrupt::RegisterHandler(Interrupt::IRQ4, UARTIRQHandler, NULL);

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
		CPU::OutPortB(port + FCR, 0);
		CPU::OutPortB(port + LCR, 0x80);
		CPU::OutPortB(port + DLL, 0xC);
		CPU::OutPortB(port + DLM, 0x0);
		CPU::OutPortB(port + LCR, 0x3); // 8n1
		CPU::OutPortB(port + MCR, 0x3); // DTR + RTS
		CPU::OutPortB(port + IER, interrupts);
	}
}

} // namespace COM
} // namespace Sortix
