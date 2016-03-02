/*
 * Copyright (c) 2011 Jonas 'Sortie' Termansen.
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
 * uart.cpp
 * A simple serial terminal driver.
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <sortix/kernel/cpu.h>
#include <sortix/kernel/ioport.h>
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
	uint8_t lcr = inport8(port + LCR);
	outport8(port + LCR, lcr | LCR_DLAB);
	uint8_t dll = inport8(port + DLL);
	uint8_t dlm = inport8(port + DLM);
	outport8(port + LCR, lcr);
	unsigned quot = dlm << 8 | dll;

	return BASE_BAUD / quot;
}

void WaitForEmptyBuffers(unsigned port)
{
	while ( (inport8(port + LSR) & BOTH_EMPTY) != BOTH_EMPTY );
}

const unsigned PORT = 0x3F8;

void Init()
{
	unsigned baud = ProbeBaud(PORT);

	outport8(PORT + LCR, 0x3); // 8n1
	outport8(PORT + IER, 0); // No interrupt
	outport8(PORT + FCR, 0); // No FIFO
	outport8(PORT + MCR, 0x3); // DTR + RTS

	unsigned divisor = 115200 / baud;
	uint8_t c = inport8(PORT + LCR);
	outport8(PORT + LCR, c | LCR_DLAB);
	outport8(PORT + DLL, divisor >> 0 & 0xFF);
	outport8(PORT + DLM, divisor >> 8 & 0xFF);
	outport8(PORT + LCR, c & ~LCR_DLAB);
}

void Read(uint8_t* buffer, size_t size)
{
	// Save the IER and disable interrupts.
	unsigned ier = inport8(PORT + IER);
	outport8(PORT + IER, 0);

	for ( size_t i = 0; i < size; i++ )
	{
		while ( !(inport8(PORT + LSR) & LSR_READY) );
		buffer[i] = inport8(PORT);
	}

	// Wait for transmitter to become empty and restore the IER.
	WaitForEmptyBuffers(PORT);
	outport8(PORT + IER, ier);
}

void Write(const void* b, size_t size)
{
	const uint8_t* buffer = (const uint8_t*) b;

	// Save the IER and disable interrupts.
	unsigned ier = inport8(PORT + IER);
	outport8(PORT + IER, 0);

	for ( size_t i = 0; i < size; i++ )
	{
		WaitForEmptyBuffers(PORT);
		outport8(PORT, buffer[i]);
	}

	// Wait for transmitter to become empty and restore the IER.
	WaitForEmptyBuffers(PORT);
	outport8(PORT + IER, ier);
}

void WriteChar(char c)
{
	// Save the IER and disable interrupts.
	unsigned ier = inport8(PORT + IER);
	outport8(PORT + IER, 0);

	WaitForEmptyBuffers(PORT);

	outport8(PORT, c);

	// Wait for transmitter to become empty and restore the IER.
	WaitForEmptyBuffers(PORT);
	outport8(PORT + IER, ier);
}

int TryPopChar()
{
	// Save the IER and disable interrupts.
	unsigned ier = inport8(PORT + IER);
	outport8(PORT + IER, 0);

	int result = -1;

	if ( inport8(PORT + LSR) & LSR_READY )
		result = inport8(PORT);

	// Wait for transmitter to become empty and restore the IER.
	WaitForEmptyBuffers(PORT);
	outport8(PORT + IER, ier);

	return result;
}

} // namespace UART
} // namespace Sortix
