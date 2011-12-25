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
.globl start, _start

.section .text

.type _beginkernel, @function
beginkernel:
_beginkernel:
	movw $0x736, 0xB83E8
	movw $0x734, 0xB83EA
	movw $0x753, 0xB83EE
	movw $0x74F, 0xB83F0
	movw $0x752, 0xB83F2
	movw $0x754, 0xB83F4
	movw $0x749, 0xB83F6
	movw $0x758, 0xB83F8

	# Initialize the stack pointer.
	movq $stack, %rsp
	addq $65536, %rsp # 64 KiB, see kernel.cpp

	# Reset EFLAGS.
	# pushl	$0
	# popf

	# Push the pointer to the Multiboot information structure.
	mov %rbx, %rsi
	# Push the magic value.
	mov %rax, %rdi

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

