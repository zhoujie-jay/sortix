/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011.

	This file is part of LibMaxsi.

	LibMaxsi is free software: you can redistribute it and/or modify it under
	the terms of the GNU Lesser General Public License as published by the Free
	Software Foundation, either version 3 of the License, or (at your option)
	any later version.

	LibMaxsi is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
	more details.

	You should have received a copy of the GNU Lesser General Public License
	along with LibMaxsi. If not, see <http://www.gnu.org/licenses/>.

	signalnum.h
	Declares the signal-related constants.

******************************************************************************/

#ifndef LIBMAXSI_SIGNALNUM_H
#define LIBMAXSI_SIGNALNUM_H

namespace Maxsi
{
	namespace Signal
	{
		const int HUP = 1; /* Hangup */
		const int INT = 2; /* Interrupt */
		const int QUIT = 3; /* Quit */
		const int ILL = 4; /* Illegal Instruction */
		const int TRAP = 5; /* Trace/Breakpoint Trap */
		const int ABRT = 6; /* Abort */
		const int EMT = 7; /* Emulation Trap */
		const int FPE = 8; /* Arithmetic Exception */
		const int KILL = 9; /* Killed */
		const int BUS = 10; /* Bus Error */
		const int SEGV = 11; /* Segmentation Fault */
		const int SYS = 12; /* Bad System Call */
		const int PIPE = 13; /* Broken Pipe */
		const int ALRM = 14; /* Alarm Clock */
		const int TERM = 15; /* Terminated */
		const int USR1 = 16; /* User Signal 1 */
		const int USR2 = 17; /* User Signal 2 */
		const int CHLD = 18; /* Child Status */
		const int PWR = 19; /* Power Fail/Restart */
		const int WINCH = 20; /* Window Size Change */
		const int URG = 21; /* Urgent Socket Condition */
		const int STOP = 23; /* Stopped (signal) */
		const int TSTP = 24; /* Stopped (user) */
		const int CONT = 25; /* Continued */
		const int TTIN = 26; /* Stopped (tty input) */
		const int TTOU = 27; /* Stopped (tty output) */
		const int VTALRM = 28; /* Virtual Timer Expired */
		const int XCPU = 30; /* CPU time limit exceeded */
		const int XFSZ = 31; /* File size limit exceeded */
		const int WAITING = 32; /* All LWPs blocked */
		const int LWP = 33; /* Virtual Interprocessor Interrupt for Threads Library */
		const int AIO = 34; /* Asynchronous I/O */
		const int NUMSIGNALS = 35;

		typedef void (*Handler)(int);
	}
}

#endif

