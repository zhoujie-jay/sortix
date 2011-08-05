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

.globl start, _start

.section .text

.text  0x100000
.type _start, @function
start:
_start:
	jmp beginkernel

	# Align 32 bits boundary.
	.align	4

	# Multiboot header.
multiboot_header:
	# Magic.
	.long	0x1BADB002
	# Flags.
	.long	0x00000003
	# Checksum.
	.long	-(0x1BADB002 + 0x00000003)

