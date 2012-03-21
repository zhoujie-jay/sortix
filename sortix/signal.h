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

	signal.h
	Classes and functions making it easier to handle Unix signals.

******************************************************************************/

#ifndef SORTIX_SIGNAL_H
#define SORTIX_SIGNAL_H

#include <libmaxsi/signalnum.h>
#include "cpu.h"

namespace Sortix
{
	#define SIGHUP		1 /* Hangup */
	#define SIGINT		2 /* Interrupt */
	#define SIGQUIT		3 /* Quit */
	#define SIGILL		4 /* Illegal Instruction */
	#define SIGTRAP		5 /* Trace/Breakpoint Trap */
	#define SIGABRT		6 /* Abort */
	#define SIGEMT		7 /* Emulation Trap */
	#define SIGFPE		8 /* Arithmetic Exception */
	#define SIGKILL		9 /* Killed */
	#define SIGBUS		10 /* Bus Error */
	#define SIGSEGV		11 /* Segmentation Fault */
	#define SIGSYS		12 /* Bad System Call */
	#define SIGPIPE		13 /* Broken Pipe */
	#define SIGALRM		14 /* Alarm Clock */
	#define SIGTERM		15 /* Terminated */
	#define SIGUSR1		16 /* User Signal 1 */
	#define SIGUSR2		17 /* User Signal 2 */
	#define SIGCHLD		18 /* Child Status */
	#define SIGPWR		19 /* Power Fail/Restart */
	#define SIGWINCH	20 /* Window Size Change */
	#define SIGURG		21 /* Urgent Socket Condition */
	#define SIGSTOP		23 /* Stopped (signal) */
	#define SIGTSTP		24 /* Stopped (user) */
	#define SIGCONT		25 /* Continued */
	#define SIGTTIN		26 /* Stopped (tty input) */
	#define SIGTTOU		27 /* Stopped (tty output) */
	#define SIGVTALRM	28 /* Virtual Timer Expired */
	#define SIGXCPU		30 /* CPU time limit exceeded */
	#define SIGXFSZ		31 /* File size limit exceeded */
	#define SIGWAITING	32 /* All LWPs blocked */
	#define SIGLWP		33 /* Virtual Interprocessor Interrupt for Threads Library */
	#define SIGAIO		34 /* Asynchronous I/O */

	struct Signal
	{
		int signum;
		int returncode;
		unsigned numpending;
		CPU::InterruptRegisters regs;
		Signal* nextsignal;

	public:
		Signal* Fork();

	};

	class SignalQueue
	{
	public:
		SignalQueue();
		~SignalQueue();

	private:
		Signal* queue;
		Signal* last;

	public:
		bool Push(int signum);
		Signal* Pop(Signal* current);

	private:
		void Insert(Signal* signal);

	};
}

#endif

