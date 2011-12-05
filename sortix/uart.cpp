/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011.

	This file is part of Sortix.

	Sortix is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License as published by the Free Software
	Foundation, either version 3 of the License, or (at your option) any later
	version.

	Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
	details.

	You should have received a copy of the GNU General Public License along
	with Sortix. If not, see <http://www.gnu.org/licenses/>.

	uart.h
	A simple serial terminal driver.

******************************************************************************/

#include "platform.h"
#include <libmaxsi/string.h>
#include <libmaxsi/memory.h>
#include "vga.h"
#include "uart.h"

using namespace Maxsi;

namespace Sortix
{
	namespace UART
	{
		const nat TXR = 0; // Transmit register
		const nat RXR = 0; // Receive register
		const nat IER = 1; // Interrupt Enable 
		const nat IIR = 2; // Interrupt ID
		const nat FCR = 2; // FIFO control
		const nat LCR = 3; // Line control
		const nat MCR = 4; // Modem control
		const nat LSR = 5; // Line Status
		const nat MSR = 6; // Modem Status
		const nat DLL = 0; // Divisor Latch Low 
		const nat DLM = 1; // Divisor latch High

		const nat LCR_DLAB = 0x80; // Divisor latch access bit
		const nat LCR_SBC = 0x40; // Set break control
		const nat LCR_SPAR = 0x20; // Stick parity (?)
		const nat LCR_EPAR = 0x10; // Even parity select
		const nat LCR_PARITY = 0x08; // Parity Enable
		const nat LCR_STOP = 0x04; // Stop bits: 0=1 bit, 1=2 bits
		const nat LCR_WLEN5 = 0x00; // Wordlength: 5 bits
		const nat LCR_WLEN6 = 0x01; // Wordlength: 6 bits
		const nat LCR_WLEN7 = 0x02; // Wordlength: 7 bits
		const nat LCR_WLEN8 = 0x03; // Wordlength: 8 bits

		const nat LSR_TEMT = 0x40; // Transmitter empty
		const nat LSR_THRE = 0x20; // Transmit-hold-register empty
		const nat LSR_READY = 0x1;

		const nat Port = 0x3f8;

		const nat BASE_BAUD = 1843200/16;
		const nat BOTH_EMPTY = LSR_TEMT | LSR_THRE;

		const unsigned FrameWidth = 80;
		const unsigned FrameHeight = 25;
		uint16_t VGALastFrame[FrameWidth * FrameHeight];

		nat ProbeBaud(nat Port)
		{
			uint8_t lcr = CPU::InPortB(Port + LCR);
			CPU::OutPortB(Port + LCR, lcr | LCR_DLAB);
			uint8_t dll = CPU::InPortB(Port + DLL);
			uint8_t dlm = CPU::InPortB(Port + DLM);
			CPU::OutPortB(Port + LCR, lcr);
			nat quot = (dlm << 8) | dll;

			return BASE_BAUD / quot;
		}

		void WaitForEmptyBuffers(nat Port)
		{
			while ( true )
			{
				nat Status = CPU::InPortB(Port + LSR);

				if ( (Status & BOTH_EMPTY) == BOTH_EMPTY )
				{
					return;
				}
			}
		}

		nat Baud;

		void Init()
		{
			InvalidateVGA();

#ifdef JSSORTIX
			// This crashes the JS VM, so don't do it.
			return;
#endif

			Baud = ProbeBaud(Port);

			CPU::OutPortB(Port + LCR, 0x3); // 8n1
			CPU::OutPortB(Port + IER, 0); // No interrupt
			CPU::OutPortB(Port + FCR, 0); // No FIFO
			CPU::OutPortB(Port + MCR, 0x3); // DTR + RTS

			nat Divisor = 115200 / Baud;
			uint8_t C = CPU::InPortB(Port + LCR);
			CPU::OutPortB(Port + LCR, C | LCR_DLAB);
			CPU::OutPortB(Port + DLL, Divisor & 0xFF);
			CPU::OutPortB(Port + DLM, (Divisor >> 8) & 0xFF);
			CPU::OutPortB(Port + LCR, C & ~LCR_DLAB);
		}

		void Read(uint8_t* Buffer, size_t Size)
		{
			// Save the IER and disable interrupts.
			nat ier = CPU::InPortB(Port + IER);
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
			nat ier = CPU::InPortB(Port + IER);
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
			nat ier = CPU::InPortB(Port + IER);
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
			nat ier = CPU::InPortB(Port + IER);
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

		void WriteNumberAsString(uint8_t Num)
		{
			if ( Num > 100 ) { WriteChar(Num / 100); }
			if ( Num > 10 ) { WriteChar(Num / 10); }

			WriteChar(Num % 10);
		}

		// Change from VGA color to another color system.
		nat ConversionTable[16] = { 0, 4, 2, 6, 1, 5, 3, 7, 0, 4, 2, 6, 1, 5, 3, 7 };

		void InvalidateVGA()
		{
			for ( nat I = 0; I < FrameWidth * FrameHeight; I++ ) { VGALastFrame[I] = 0; }	
		}

		void RenderVGA(const uint16_t* Frame)
		{
			const uint16_t* Source = Frame;

			nat LastColor = 1337;
			nat SkippedSince = 0;
			bool posundefined = true;

			for ( nat Y = 0; Y < FrameHeight; Y++)
			{
				for ( nat X = 0; X < FrameWidth; X++ )
				{
					nat Index = Y * FrameWidth + X;

					nat Element = Source[Index];
					nat OldElement = VGALastFrame[Index];

					if ( Element == OldElement ) { continue; }

					// Update the position if we skipped some characters.
					if ( Index - SkippedSince > 8 || posundefined )
					{
						const nat LineId = Y + 1;
						const nat ColumnId = X + 1;

						if ( ColumnId > 1 )
						{
							UART::WriteChar('\e');
							UART::WriteChar('[');
							UART::WriteChar('0' + LineId / 10);
							UART::WriteChar('0' + LineId % 10);
							UART::WriteChar(';');
							UART::WriteChar('0' + ColumnId / 10);
							UART::WriteChar('0' + ColumnId % 10);
							UART::WriteChar('H');
						}
						else
						{
							UART::WriteChar('\e');
							UART::WriteChar('[');
							UART::WriteChar('0' + LineId / 10);
							UART::WriteChar('0' + LineId % 10);
							UART::WriteChar('H');
						}

						SkippedSince = Index;
						posundefined = false;
					}

					for ( nat Pos = SkippedSince; Pos <= Index; Pos++ )
					{
						Element = Source[Pos];
						OldElement = VGALastFrame[Pos];

						nat NewColor = (ConversionTable[ (Element >> 12) & 0xF ] << 3) | (ConversionTable[ (Element >> 8) & 0xF ]);
		
						// Change the color if we need to.
						if ( LastColor != NewColor )
						{
							nat OldFGColor = LastColor % 8;
							nat OldBGColor = LastColor / 8;
							nat FGColor = NewColor % 8;
							nat BGColor = NewColor / 8;
							if ( LastColor == 1337 ) { OldFGColor = 9; OldBGColor = 9; }

							if ( (OldFGColor != FGColor) && (OldBGColor != BGColor) )
							{
								UART::WriteChar('\e');
								UART::WriteChar('[');
								UART::WriteChar('3');
								UART::WriteChar('0' + FGColor);
								UART::WriteChar(';');
								UART::WriteChar('4');
								UART::WriteChar('0' + BGColor);
								UART::WriteChar('m');
							}
							else if ( OldFGColor != FGColor )
							{
								UART::WriteChar('\e');
								UART::WriteChar('[');
								UART::WriteChar('3');
								UART::WriteChar('0' + FGColor);
								UART::WriteChar('m');
							}
							else if ( OldBGColor != BGColor )
							{
								UART::WriteChar('\e');
								UART::WriteChar('[');
								UART::WriteChar('4');
								UART::WriteChar('0' + BGColor);
								UART::WriteChar('m');
							}

							LastColor = NewColor;
						}

						VGALastFrame[Pos] = Element;

						Element &= 0x7F;

						// Filter away any non-printable characters.
						if ( Element < 32 || Element > 126 ) { Element = '?'; }

						UART::WriteChar(Element);
					}

					SkippedSince = Index + 1;
				}
			}
		}
	}
}
