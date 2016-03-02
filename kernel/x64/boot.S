/*
 * Copyright (c) 2011, 2014, 2015 Jonas 'Sortie' Termansen.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * x64/boot.S
 * Bootstraps the kernel and passes over control from the boot-loader to the
 * kernel main function. It also jumps into long mode!
 */

.section .text
.text 0x100000

	# Multiboot header.
	.align 4
	.long 0x1BADB002 # Magic.
	.long 0x00000007 # Flags.
	.long -(0x1BADB002 + 0x00000007) # Checksum
	.skip 32-12
	.long 0 # Mode
	.long 0 # Width
	.long 0 # Height
	.long 0 # Depth

.section .bss, "aw", @nobits
	.align 4096
bootpml4:
	.skip 4096
bootpml3:
	.skip 4096
bootpml2:
	.skip 4096
bootpml1_a:
	.skip 4096
bootpml1_b:
	.skip 4096
fracpml3:
	.skip 4096
fracpml2:
	.skip 4096
fracpml1:
	.skip 4096
forkpml2:
	.skip 4096
forkpml1:
	.skip 4096
physpml3:
	.skip 4096
physpml2:
	.skip 4096
physpml1:
	.skip 4096
physpml0:
	.skip 4096
nullpage: .global nullpage
	.skip 4096

.section .text
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

	movl $bootpml4, %edi
	movl %edi, %cr3

	# Page-Map Level 4.
	movl $(bootpml3 + 0x207), bootpml4 + 0 * 8

	# Page Directory Pointer Table.
	movl $(bootpml2 + 0x207), bootpml3 + 0 * 8

	# Page Directory (no user-space access here).
	movl $(bootpml1_a + 0x003), bootpml2 + 0 * 8
	movl $(bootpml1_b + 0x003), bootpml2 + 1 * 8

	# Page Table (identity map the first 4 MiB, except NULL).
	# TODO: This is insecure as it doesn't restrict write & execute access to
	#       the code kernel code & variables appropriately.
	movl $(bootpml1_a + 8), %edi
	movl $0x1003, %esi
	movl $1023, %ecx
1:
	movl %esi, (%edi)
	addl $0x1000, %esi
	addl $8, %edi
	loop 1b

	# Map the null page.
	movl $nullpage, %edi
	shrl $12, %edi
	movl $0x0003, bootpml1_a(, %edi, 8)

	# Fractal mapping.
	movl $(bootpml4 + 0x003), bootpml4 + 511 * 8
	movl $(fracpml3 + 0x203), bootpml4 + 510 * 8
	movl $(bootpml4 + 0x003), fracpml3 + 511 * 8
	movl $(fracpml2 + 0x203), fracpml3 + 510 * 8
	movl $(bootpml4 + 0x003), fracpml2 + 511 * 8
	movl $(fracpml1 + 0x203), fracpml2 + 510 * 8
	movl $(bootpml4 + 0x003), fracpml1 + 511 * 8

	# Predefined room for forking address spaces.
	movl $(forkpml2 + 0x203), fracpml3 + 0 * 8
	movl $(forkpml1 + 0x203), forkpml2 + 0 * 8

	# Physical page allocator.
	movl $(physpml3 + 0x003), bootpml4 + 509 * 8
	movl $(physpml2 + 0x003), physpml3 + 0   * 8
	movl $(physpml1 + 0x003), physpml2 + 0   * 8
	movl $(physpml0 + 0x003), physpml1 + 0   * 8

	# Enable PAE.
	movl %cr4, %eax
	orl $0x20, %eax
	movl %eax, %cr4

	# Enable long mode and the No-Execute bit.
	movl $0xC0000080, %ecx
	rdmsr
	orl $0x900, %eax
	wrmsr

	# Enable paging (with write protection) and enter long mode (still 32-bit)
	movl %cr0, %eax
	orl $0x80010000, %eax
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
