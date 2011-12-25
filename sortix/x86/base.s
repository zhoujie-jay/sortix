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

	boot.s
	Bootstraps the kernel and passes over control from the boot-loader to the
	kernel main function.

******************************************************************************/

.globl beginkernel, _beginkernel

.section .text

.text  0x100000
.type _beginkernel, @function
beginkernel:
_beginkernel:
	# Initialize the stack pointer. The magic value is from kernel.cpp.
	movl $stack, %esp
	addl $65536, %esp # 64 KiB, see kernel.cpp

	# Reset EFLAGS.
	# pushl	$0
	# popf

	# Push the pointer to the Multiboot information structure.
	push	%ebx
	# Push the magic value.
	push	%eax

	## Disable interrupts.
	cli

	call KernelInit

.globl HaltKernel
HaltKernel:
	cli
	hlt
	jmp HaltKernel

.globl WaitForInterrupt
.type WaitForInterrupt, @function # void WaitForInterrupt();
WaitForInterrupt:
	hlt
	ret
