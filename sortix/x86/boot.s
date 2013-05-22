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

.section .text
.text 0x100000

.global _start
.type _start, @function
_start:
	jmp prepare_kernel_execution

	# Align 32 bits boundary.
	.align 4

	# Multiboot header.
multiboot_header:
	# Magic.
	.long 0x1BADB002
	# Flags.
	.long 0x00000003
	# Checksum.
	.long -(0x1BADB002 + 0x00000003)

prepare_kernel_execution:
	# Enable the floating point unit.
	mov %eax, 0x100000
	mov %cr0, %eax
	and $0xFFFD, %ax
	or $0x10, %ax
	mov %eax, %cr0
	fninit

	# Enable Streaming SIMD Extensions.
	mov %cr0, %eax
	and $0xFFFB, %ax
	or $0x2, %ax
	mov %eax, %cr0
	mov %cr4, %eax
	or $0x600, %eax
	mov %eax, %cr4
	mov 0x100000, %eax

	jmp beginkernel
