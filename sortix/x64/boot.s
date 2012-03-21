/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011.

	This file is part of Sortix.

	Sortix is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License as published by the Free Software
	Foundation, either version 3 of the License, or (at your option) any later
	version.

	Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY# without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
	details.

	You should have received a copy of the GNU General Public License along
	with Sortix. If not, see <http://www.gnu.org/licenses/>.

	boot.s
	Bootstraps the kernel and passes over control from the boot-loader to the
	kernel main function. It also jumps into long mode!

******************************************************************************/

.globl start, _start

.section .text
.text  0x100000
.type _start, @function
.code32
start:
_start:
	jmp multiboot_entry

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

multiboot_entry:

	# We got our multiboot information in various registers. But we are going
	# to need these registers. But where can we store them then? Oh hey, let's
	# store then in the code already run!

	# Store the pointer to the Multiboot information structure.
	mov %ebx, 0x100000

	# Store the magic value.
	mov %eax, 0x100004

	# Clear the first $0xE000 bytes following 0x21000.
	movl $0x21000, %edi
	mov %edi, %cr3
	xorl %eax, %eax
	movl $0xE000, %ecx
	rep stosl
	movl %cr3, %edi

	# Set the initial page tables.
	# Note that we OR with 0x7 here to allow user-space access, except in the
	# first 2 MiB. We also do this with 0x200 to allow forking the page.

	# Page-Map Level 4
	movl $0x22207, (%edi)
	addl $0x1000, %edi

	# Page-Directory Pointer Table
	movl $0x23207, (%edi)
	addl $0x1000, %edi

	# Page-Directory (no user-space access here)
	movl $0x24003, (%edi) # (First 2 MiB)
	movl $0x25003, 8(%edi) # (Second 2 MiB)
	addl $0x1000, %edi

	# Page-Table
	# Memory map the first 4 MiB.
	movl $0x3, %ebx
	movl $1024, %ecx

SetEntry:
	mov %ebx, (%edi)
	add $0x1000, %ebx
	add $8, %edi
	loop SetEntry

	# Enable PAE.
	mov %cr4, %eax
	orl $0x20, %eax
	mov %eax, %cr4

	# Enable long mode.
	mov $0xC0000080, %ecx
	rdmsr
	orl $0x100, %eax
	wrmsr

	# Enable paging and enter long mode (still 32-bit)
	mov %cr0, %eax
	orl $0x80000000, %eax
	mov %eax, %cr0

	# Load the long mode GDT.
	mov GDTPointer, %eax
	lgdtl GDTPointer

	# Now use the 64-bit code segment, and we are in full 64-bit mode.
	ljmp $0x10, $Realm64

.code64
Realm64:

	# Now, set up the other segment registers.
	cli
	mov $0x18, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	# Alright, that was the bootstrap code. Now begin preparing to run the
	# actual 64-bit kernel.
	jmp Main

.section .data
GDT64:                           # Global Descriptor Table (64-bit).
	GDTNull:                       # The null descriptor.
	.word 0                         # Limit (low).
	.word 0                         # Base (low).
	.byte 0                         # Base (middle)
	.byte 0                         # Access.
	.byte 0                         # Granularity.
	.byte 0                         # Base (high).
	GDTUnused:                       # The null descriptor.
	.word 0                         # Limit (low).
	.word 0                         # Base (low).
	.byte 0                         # Base (middle)
	.byte 0                         # Access.
	.byte 0                         # Granularity.
	.byte 0                         # Base (high).
	GDTCode:                       # The code descriptor.
	.word 0xFFFF                    # Limit (low).
	.word 0                         # Base (low).
	.byte 0                         # Base (middle)
	.byte 0x9A                      # Access.
	.byte 0xAF                      # Granularity.
	.byte 0                         # Base (high).
	GDTData:                       # The data descriptor.
	.word 0xFFFF                    # Limit (low).
	.word 0                         # Base (low).
	.byte 0                         # Base (middle)
	.byte 0x92                      # Access.
	.byte 0x8F                      # Granularity.
	.byte 0                         # Base (high).
	GDTPointer:                    # The GDT-pointer.
	.word GDTPointer - GDT64 - 1     # Limit.
	.long GDT64                      # Base.
	.long 0

Main:
	# Copy the character B onto the screen so we know it works.
	movq $0x242, %r15
	movq %r15, %rax
	movw %ax, 0xB8000

	# Load the pointer to the Multiboot information structure.
	mov 0x100000, %ebx

	# Load the magic value.
	mov 0x100004, %eax

	jmp beginkernel

