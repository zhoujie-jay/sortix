/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011.

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

    uart.cpp
    A simple serial terminal driver.

*******************************************************************************/

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <sortix/kernel/cpu.h>
#include <sortix/kernel/kernel.h>

#include "vga.h"
#include "uart.h"

namespace Sortix {
namespace UART {

const unsigned TXR = 0; // Transmit register
const unsigned RXR = 0; // Receive register
const unsigned IER = 1; // Interrupt Enable
const unsigned IIR = 2; // Interrupt ID
const unsigned FCR = 2; // FIFO control
const unsigned LCR = 3; // Line control
const unsigned MCR = 4; // Modem control
const unsigned LSR = 5; // Line Status
const unsigned MSR = 6; // Modem Status
const unsigned DLL = 0; // divisor Latch Low
const unsigned DLM = 1; // divisor latch High

const unsigned LCR_DLAB = 0x80; // divisor latch access bit
const unsigned LCR_SBC = 0x40; // Set break control
const unsigned LCR_SPAR = 0x20; // Stick parity (?)
const unsigned LCR_EPAR = 0x10; // Even parity select
const unsigned LCR_PARITY = 0x08; // Parity Enable
const unsigned LCR_STOP = 0x04; // Stop bits: 0=1 bit, 1=2 bits
const unsigned LCR_WLEN5 = 0x00; // Wordlength: 5 bits
const unsigned LCR_WLEN6 = 0x01; // Wordlength: 6 bits
const unsigned LCR_WLEN7 = 0x02; // Wordlength: 7 bits
const unsigned LCR_WLEN8 = 0x03; // Wordlength: 8 bits

const unsigned LSR_TEMT = 0x40; // Transmitter empty
const unsigned LSR_THRE = 0x20; // Transmit-hold-register empty
const unsigned LSR_READY = 0x1;

const unsigned BASE_BAUD = 1843200 / 16;
const unsigned BOTH_EMPTY = LSR_TEMT | LSR_THRE;

unsigned ProbeBaud(unsigned port)
{
	uint8_t lcr = CPU::InPortB(port + LCR);
	CPU::OutPortB(port + LCR, lcr | LCR_DLAB);
	uint8_t dll = CPU::InPortB(port + DLL);
	uint8_t dlm = CPU::InPortB(port + DLM);
	CPU::OutPortB(port + LCR, lcr);
	unsigned quot = dlm << 8 | dll;

	return BASE_BAUD / quot;
}

void WaitForEmptyBuffers(unsigned port)
{
	while ( (CPU::InPortB(port + LSR) & BOTH_EMPTY) != BOTH_EMPTY );
}

const unsigned PORT = 0x3F8;

void Init()
{
	unsigned baud = ProbeBaud(PORT);

	CPU::OutPortB(PORT + LCR, 0x3); // 8n1
	CPU::OutPortB(PORT + IER, 0); // No interrupt
	CPU::OutPortB(PORT + FCR, 0); // No FIFO
	CPU::OutPortB(PORT + MCR, 0x3); // DTR + RTS

	unsigned divisor = 115200 / baud;
	uint8_t c = CPU::InPortB(PORT + LCR);
	CPU::OutPortB(PORT + LCR, c | LCR_DLAB);
	CPU::OutPortB(PORT + DLL, divisor >> 0 & 0xFF);
	CPU::OutPortB(PORT + DLM, divisor >> 8 & 0xFF);
	CPU::OutPortB(PORT + LCR, c & ~LCR_DLAB);
}

void Read(uint8_t* buffer, size_t size)
{
	// Save the IER and disable interrupts.
	unsigned ier = CPU::InPortB(PORT + IER);
	CPU::OutPortB(PORT + IER, 0);

	for ( size_t i = 0; i < size; i++ )
	{
		while ( !(CPU::InPortB(PORT + LSR) & LSR_READY) );
		buffer[i] = CPU::InPortB(PORT);
	}

	// Wait for transmitter to become empty and restore the IER.
	WaitForEmptyBuffers(PORT);
	CPU::OutPortB(PORT + IER, ier);
}

void Write(const void* b, size_t size)
{
	const uint8_t* buffer = (const uint8_t*) b;

	// Save the IER and disable interrupts.
	unsigned ier = CPU::InPortB(PORT + IER);
	CPU::OutPortB(PORT + IER, 0);

	for ( size_t i = 0; i < size; i++ )
	{
		WaitForEmptyBuffers(PORT);
		CPU::OutPortB(PORT, buffer[i]);
	}

	// Wait for transmitter to become empty and restore the IER.
	WaitForEmptyBuffers(PORT);
	CPU::OutPortB(PORT + IER, ier);
}

void WriteChar(char c)
{
	// Save the IER and disable interrupts.
	unsigned ier = CPU::InPortB(PORT + IER);
	CPU::OutPortB(PORT + IER, 0);

	WaitForEmptyBuffers(PORT);

	CPU::OutPortB(PORT, c);

	// Wait for transmitter to become empty and restore the IER.
	WaitForEmptyBuffers(PORT);
	CPU::OutPortB(PORT + IER, ier);
}

int TryPopChar()
{
	// Save the IER and disable interrupts.
	unsigned ier = CPU::InPortB(PORT + IER);
	CPU::OutPortB(PORT + IER, 0);

	int result = -1;

	if ( CPU::InPortB(PORT + LSR) & LSR_READY )
		result = CPU::InPortB(PORT);

	// Wait for transmitter to become empty and restore the IER.
	WaitForEmptyBuffers(PORT);
	CPU::OutPortB(PORT + IER, ier);

	return result;
}

} // namespace UART
} // namespace Sortix
