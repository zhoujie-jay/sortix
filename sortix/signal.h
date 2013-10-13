/******************************************************************************

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

	You should have received a copy of the GNU General Public License along
	with Sortix. If not, see <http://www.gnu.org/licenses/>.

	signal.h
	Classes and functions making it easier to handle Unix signals.

******************************************************************************/

#ifndef SORTIX_SIGNAL_H
#define SORTIX_SIGNAL_H

#include "cpu.h"

namespace Sortix {

extern "C" volatile unsigned long asm_signal_is_pending;

namespace Signal {

class Queue
{
public:
	Queue();

// TODO: This is the wrong data structure for the problem!
private:
	bool pending[SIG_MAX_NUM];

// TODO: This is not SMP ready:
// To avoid race conditions, these should be called with interrupts off.
public:
	void Push(int signum);
	int Pop(int cursig);

};

void Init();
int Priority(int signum);
void Dispatch(CPU::InterruptRegisters* regs, void* user = NULL);
void Return(CPU::InterruptRegisters* regs, void* user = NULL);
inline bool IsPending() { return asm_signal_is_pending != 0; }

} // namespace Signal
} // namespace Sortix

#endif
