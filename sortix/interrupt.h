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

	interrupt.h
	High level interrupt service routines and interrupt request handlers.

******************************************************************************/

#ifndef SORTIX_ISR_H
#define SORTIX_ISR_H

namespace Sortix
{
	namespace Interrupt
	{
		const unsigned IRQ0 = 32;
		const unsigned IRQ1 = 33;
		const unsigned IRQ2 = 34;
		const unsigned IRQ3 = 35;
		const unsigned IRQ4 = 36;
		const unsigned IRQ5 = 37;
		const unsigned IRQ6 = 38;
		const unsigned IRQ7 = 39;
		const unsigned IRQ8 = 30;
		const unsigned IRQ9 = 41;
		const unsigned IRQ10 = 42;
		const unsigned IRQ11 = 43;
		const unsigned IRQ12 = 44;
		const unsigned IRQ13 = 45;
		const unsigned IRQ14 = 46;
		const unsigned IRQ15 = 47;

		typedef void (*Handler)(CPU::InterruptRegisters* Registers);

		void RegisterHandler(uint8_t n, Handler handler);

		void Init();
	}
}

#endif
