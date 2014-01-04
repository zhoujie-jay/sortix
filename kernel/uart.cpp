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

#include <sortix/kernel/cpu.h>
#include <sortix/kernel/kernel.h>

#include <string.h>
#include "vga.h"
#include "uart.h"

namespace Sortix
{
	namespace UART
	{
		const unsigned TXR = 0; // Transmit register
		const unsigned RXR = 0; // Receive register
		const unsigned IER = 1; // Interrupt Enable
		const unsigned IIR = 2; // Interrupt ID
		const unsigned FCR = 2; // FIFO control
		const unsigned LCR = 3; // Line control
		const unsigned MCR = 4; // Modem control
		const unsigned LSR = 5; // Line Status
		const unsigned MSR = 6; // Modem Status
		const unsigned DLL = 0; // Divisor Latch Low
		const unsigned DLM = 1; // Divisor latch High

		const unsigned LCR_DLAB = 0x80; // Divisor latch access bit
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

		const unsigned Port = 0x3f8;

		const unsigned BASE_BAUD = 1843200/16;
		const unsigned BOTH_EMPTY = LSR_TEMT | LSR_THRE;

		unsigned ProbeBaud(unsigned Port)
		{
			uint8_t lcr = CPU::InPortB(Port + LCR);
			CPU::OutPortB(Port + LCR, lcr | LCR_DLAB);
			uint8_t dll = CPU::InPortB(Port + DLL);
			uint8_t dlm = CPU::InPortB(Port + DLM);
			CPU::OutPortB(Port + LCR, lcr);
			unsigned quot = (dlm << 8) | dll;

			return BASE_BAUD / quot;
		}

		void WaitForEmptyBuffers(unsigned Port)
		{
			while ( true )
			{
				unsigned Status = CPU::InPortB(Port + LSR);

				if ( (Status & BOTH_EMPTY) == BOTH_EMPTY )
				{
					return;
				}
			}
		}

		unsigned Baud;

		void Init()
		{
			Baud = ProbeBaud(Port);

			CPU::OutPortB(Port + LCR, 0x3); // 8n1
			CPU::OutPortB(Port + IER, 0); // No interrupt
			CPU::OutPortB(Port + FCR, 0); // No FIFO
			CPU::OutPortB(Port + MCR, 0x3); // DTR + RTS

			unsigned Divisor = 115200 / Baud;
			uint8_t C = CPU::InPortB(Port + LCR);
			CPU::OutPortB(Port + LCR, C | LCR_DLAB);
			CPU::OutPortB(Port + DLL, Divisor & 0xFF);
			CPU::OutPortB(Port + DLM, (Divisor >> 8) & 0xFF);
			CPU::OutPortB(Port + LCR, C & ~LCR_DLAB);
		}

		void Read(uint8_t* Buffer, size_t Size)
		{
			// Save the IER and disable interrupts.
			unsigned ier = CPU::InPortB(Port + IER);
			CPU::OutPortB(Port + IER, 0);

			for ( size_t I = 0; I < Size; I++ )
			{
				while ( ! ( CPU::InPortB(Port + LSR) & LSR_READY ) ) { }

				Buffer[I] = CPU::InPortB(Port);
			}

			// Wait for transmitter to become empty and restore the IER.
			WaitForEmptyBuffers(Port);
			CPU::OutPortB(Port + IER, ier);
		}

		void Write(const void* B, size_t Size)
		{
			const uint8_t* Buffer = (const uint8_t*) B;

			// Save the IER and disable interrupts.
			unsigned ier = CPU::InPortB(Port + IER);
			CPU::OutPortB(Port + IER, 0);

			for ( size_t I = 0; I < Size; I++ )
			{
				WaitForEmptyBuffers(Port);

				CPU::OutPortB(Port, Buffer[I]);
			}

			// Wait for transmitter to become empty and restore the IER.
			WaitForEmptyBuffers(Port);
			CPU::OutPortB(Port + IER, ier);
		}

		void WriteChar(char C)
		{
			// Save the IER and disable interrupts.
			unsigned ier = CPU::InPortB(Port + IER);
			CPU::OutPortB(Port + IER, 0);

			WaitForEmptyBuffers(Port);

			CPU::OutPortB(Port, C);

			// Wait for transmitter to become empty and restore the IER.
			WaitForEmptyBuffers(Port);
			CPU::OutPortB(Port + IER, ier);
		}

		int TryPopChar()
		{
			// Save the IER and disable interrupts.
			unsigned ier = CPU::InPortB(Port + IER);
			CPU::OutPortB(Port + IER, 0);

			int Result = -1;

			if ( CPU::InPortB(Port + LSR) & LSR_READY )
			{
				Result = CPU::InPortB(Port);
			}

			// Wait for transmitter to become empty and restore the IER.
			WaitForEmptyBuffers(Port);
			CPU::OutPortB(Port + IER, ier);

			return Result;
		}
	}
}
