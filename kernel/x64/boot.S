/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2014, 2015.

    This file is part of Sortix.

    Sortix is free software: you can redistribute it and/or modify it under the
    terms of the GNU General Public License as published by the Free Software
    Foundation, either version 3 of the License, or (at your option) any later
    version.

    Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
    details.

    You should have received a copy of the GNU General Public License along with
    Sortix. If not, see <http://www.gnu.org/licenses/>.

    x64/boot.S
    Bootstraps the kernel and passes over control from the boot-loader to the
    kernel main function. It also jumps into long mode!

*******************************************************************************/

.section .text
.text 0x100000

	# Multiboot header.
	.align 4
	.long 0x1BADB002 # Magic.
	.long 0x00000003 # Flags.
	.long -(0x1BADB002 + 0x00000003) # Checksum

.global _start
.global __start
.type _start, @function
.type __start, @function
.code32
_start:
__start:
	# Initialize the stack pointer. The magic value is from kernel.cpp.
	movl $(stack + 65536), %esp # 64 KiB, see kernel.cpp (See below also)

	# Finish installing the kernel stack into the Task Switch Segment.
	movl %esp, tss + 4
	movl $0, tss + 8

	# Finish installing the Task Switch Segment into the Global Descriptor Table.
	movl $tss, %ecx
	movw %cx, gdt + 0x28 + 2
	shrl $16, %ecx
	movb %cl, gdt + 0x28 + 4
	shrl $8, %ecx
	movb %cl, gdt + 0x28 + 7
	movl $0, gdt + 0x28 + 8

	# We got our multiboot information in various registers.
	pushl $0
	pushl %eax # Multiboot magic value.
	pushl $0
	pushl %ebx # Multiboot information structure pointer.

	# Clear the first $0xE000 bytes following 0x21000.
	movl $0x21000, %edi
	movl %edi, %cr3
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

1:
	movl %ebx, (%edi)
	addl $0x1000, %ebx
	addl $8, %edi
	loop 1b

	# Enable PAE.
	movl %cr4, %eax
	orl $0x20, %eax
	movl %eax, %cr4

	# Enable long mode.
	movl $0xC0000080, %ecx
	rdmsr
	orl $0x100, %eax
	wrmsr

	# Enable paging and enter long mode (still 32-bit)
	movl %cr0, %eax
	orl $0x80000000, %eax
	movl %eax, %cr0

	# Load the Global Descriptor Table pointer register.
	subl $6, %esp
	movw gdt_size_minus_one, %cx
	movw %cx, 0(%esp)
	movl $gdt, %ecx
	movl %ecx, 2(%esp)
	lgdt 0(%esp)
	addl $6, %esp

	# Now use the 64-bit code segment, and we are in full 64-bit mode.
	ljmp $0x08, $2f

.code64
2:
	# Switch ds, es, fs, gs, ss to the kernel data segment (0x10).
	movw $0x10, %cx
	movw %cx, %ds
	movw %cx, %es
	movw %cx, %ss

	# Switch the task switch segment register to the task switch segment (0x28).
	movw $(0x28 /* TSS */ | 0x3 /* RPL */), %cx
	ltr %cx

	# Switch to the thread local fs and gs segments.
	movw $(0x20 /* DS */ | 0x3 /* RPL */), %cx
	movw %cx, %fs
	movw %cx, %gs

	# Enable the floating point unit.
	mov %cr0, %rax
	and $0xFFFD, %ax
	or $0x10, %ax
	mov %rax, %cr0
	fninit

	# Enable Streaming SIMD Extensions.
	mov %cr0, %rax
	and $0xFFFB, %ax
	or $0x2, %ax
	mov %rax, %cr0
	mov %cr4, %rax
	or $0x600, %rax
	mov %rax, %cr4

	# Store a copy of the initialial floating point registers.
	fxsave fpu_initialized_regs

	# Enter the high-level kernel proper.
	pop %rsi # Multiboot information structure pointer.
	pop %rdi # Multiboot magic value.
	call KernelInit

	jmp HaltKernel
.size _start, . - _start
.size __start, . - __start
.global HaltKernel
.type HaltKernel, @function
HaltKernel:
	cli
	hlt
	jmp HaltKernel
.size HaltKernel, . - HaltKernel
