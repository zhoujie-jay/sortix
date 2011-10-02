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

	x64/gdt.cpp
	Handles initialization of the 64-bit global descriptor table.

******************************************************************************/

.section .text

.global gdt_flush
.type gdt_flush, @function
gdt_flush:
	# Load the new GDT pointer
	lgdtq (%rdi)

	# 0x10 is the offset in the GDT to our data segment
	mov $0x10, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	# 0x08 is the offset to our code segment: Far jump!
	ljmp *GDT_FLUSH_POSTJMP
gdt_flush_postjmp:
	ret

.global idt_flush
.type idt_flush, @function
idt_flush:
	# Load the IDT pointer.
	lidt (%rdi)

	# Enable interrupts
	sti
	ret

.global tss_flush
.type tss_flush, @function
tss_flush:
	# Load the index of our TSS structure - The index is 0x28, as it is the 5th
	# selector and each is 8 bytes long, but we set the bottom two bits (making
	# 0x2B) so that it has an RPL of 3, not zero.
	mov $0x2B, %ax 

	# Load the task state register.
	ltr %ax
	ret

.section .data
GDT_FLUSH_POSTJMP:
	.long gdt_flush_postjmp
	.word 0x08

