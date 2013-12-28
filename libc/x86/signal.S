/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

    This file is part of the Sortix C Library.

    The Sortix C Library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    The Sortix C Library is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
    License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with the Sortix C Library. If not, see <http://www.gnu.org/licenses/>.

    x86/signal.S
    An assembly stub that for handling unix signals.

*******************************************************************************/

.globl SignalHandlerAssembly

.section .text

.type SignalHandlerAssembly, @function
SignalHandlerAssembly:

	# The kernel put the signal id in edi.
	pushl %edi
	call SignalHandler

	# Restore the stack as it was.
	addl $4, %esp

	# Return control to the kernel, so normal execution can continue.
	int $131
.size SignalHandlerAssembly, . - SignalHandlerAssembly
