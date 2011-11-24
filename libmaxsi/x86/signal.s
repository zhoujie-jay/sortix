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

	signal.s
	An assembly stub that for handling unix signals.

******************************************************************************/

.globl SignalHandlerAssembly

.section .text

.type SignalHandlerAssembly, @function
SignalHandlerAssembly:

	# The kernel put the signal id in edi.
	pushl %edi
	call SignalHandler

	# Restore the stack as it was.
	addl $4, %esp

	# Now that the stack is intact, return control to the kernel, so normal
	# computation can resume normally.
	movl $30, %eax # SysSigReturn
	int $0x80
