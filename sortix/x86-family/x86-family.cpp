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

	x86-family.cpp
	CPU stuff for the x86 CPU family.

******************************************************************************/

#include <sortix/kernel/platform.h>

namespace Sortix
{
	namespace CPU
	{
		void OutPortB(uint16_t Port, uint8_t Value)
		{
			asm volatile ("outb %1, %0" : : "dN" (Port), "a" (Value));
		}

		void OutPortW(uint16_t Port, uint16_t Value)
		{
			asm volatile ("outw %1, %0" : : "dN" (Port), "a" (Value));
		}

		void OutPortL(uint16_t Port, uint32_t Value)
		{
			asm volatile ("outl %1, %0" : : "dN" (Port), "a" (Value));
		}

		uint8_t InPortB(uint16_t Port)
		{
			uint8_t Result;
			asm volatile("inb %1, %0" : "=a" (Result) : "dN" (Port));
			return Result;
		}

		uint16_t InPortW(uint16_t Port)
		{
			uint16_t Result;
			asm volatile("inw %1, %0" : "=a" (Result) : "dN" (Port));
			return Result;
		}

		uint32_t InPortL(uint16_t Port)
		{
			uint32_t Result;
			asm volatile("inl %1, %0" : "=a" (Result) : "dN" (Port));
			return Result;
		}

		void Reboot()
		{
			// Keyboard interface IO port: data and control.
			const uint16_t KEYBOARD_INTERFACE = 0x64;

			// Keyboard IO port.
			const uint16_t KEYBOARD_IO = 0x60;

			// Keyboard data is in buffer (output buffer is empty) (bit 0).
			const uint8_t KEYBOARD_DATA = (1<<0);

			// User data is in buffer (command buffer is empty) (bit 1).
			const uint8_t USER_DATA = (1<<1);

			// Disable interrupts.
			asm volatile("cli");

			// Clear all keyboard buffers (output and command buffers).
			uint8_t byte;
			do
			{
				byte = InPortB(KEYBOARD_INTERFACE);
				if ( ( byte & KEYBOARD_DATA ) != 0 ) { InPortB(KEYBOARD_IO); }
			} while ( ( byte & USER_DATA ) != 0 );

			// CPU reset command.
			uint8_t KEYBOARD_RESET_CPU = 0xFE;

			// Now pulse the CPU reset line and reset.
			OutPortB(KEYBOARD_INTERFACE, KEYBOARD_RESET_CPU);

			// If that didn't work, just halt.
			asm volatile("hlt");
		}

		void ShutDown()
		{
			// TODO: Unimplemented, just reboot.
			Reboot();
		}
	}
}
