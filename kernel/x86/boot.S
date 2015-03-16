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

    x86/boot.S
    Bootstraps the kernel and passes over control from the boot-loader to the
    kernel main function.

*******************************************************************************/

.section .text
.text 0x100000

	# Multiboot header.
	.align 4
	.long 0x1BADB002 # Magic.
	.long 0x00000003 # Flags.
	.long -(0x1BADB002 + 0x00000003) # Checksum.

.section .bss, "aw", @nobits
	.align 4096
bootpml2:
	.skip 4096
bootpml1:
	.skip 4096
fracpml1:
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
_start:
__start:
	# Initialize the stack pointer. The magic value is from kernel.cpp.
	movl $(stack + 65536), %esp # 64 KiB, see kernel.cpp

	# Finish installing the kernel stack into the Task Switch Segment.
	movl %esp, tss + 4

	# Finish installing the Task Switch Segment into the Global Descriptor Table.
	movl $tss, %ecx
	movw %cx, gdt + 0x28 + 2
	shrl $16, %ecx
	movb %cl, gdt + 0x28 + 4
	shrl $8, %ecx
	movb %cl, gdt + 0x28 + 7

	movl $bootpml2, %edi
	movl %edi, %cr3

	# Page Directory.
	movl $(bootpml1 + 0x003), bootpml2 + 0 * 4

	# Page Table (identity map the first 4 MiB, except NULL).
	movl $(bootpml1 + 4), %edi
	movl $0x1003, %esi
	movl $1023, %ecx
1:
	movl %esi, (%edi)
	addl $0x1000, %esi
	addl $4, %edi
	loop 1b

	# Map the null page.
	movl $nullpage, %edi
	shrl $12, %edi
	movl $0x0003, bootpml1(, %edi, 4)

	# Fractal mapping.
	movl $(bootpml2 + 0x003), bootpml2 + 1023 * 4
	movl $(fracpml1 + 0x203), bootpml2 + 1022 * 4
	movl $(bootpml2 + 0x003), fracpml1 + 1023 * 4

	# Physical page allocator.
	movl $(physpml1 + 0x003), bootpml2 + 1021 * 4
	movl $(physpml0 + 0x003), physpml1 + 0    * 4

	# Enable paging.
	movl %cr0, %edi
	orl $0x80000000, %edi
	movl %edi, %cr0

	# Load the Global Descriptor Table pointer register.
	subl $6, %esp
	movw gdt_size_minus_one, %cx
	movw %cx, 0(%esp)
	movl $gdt, %ecx
	movl %ecx, 2(%esp)
	lgdt 0(%esp)
	addl $6, %esp

	# Switch cs to the kernel code segment (0x08).
	push $0x08
	push $2f
	retf

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
	movw $(0x30 /* FS */ | 0x3 /* RPL */), %cx
	movw %cx, %fs
	movw $(0x38 /* GS */ | 0x3 /* RPL */), %cx
	movw %cx, %gs

	# Enable the floating point unit.
	mov %cr0, %ecx
	and $0xFFFD, %cx
	or $0x10, %cx
	mov %ecx, %cr0
	fninit

	# Enable Streaming SIMD Extensions.
	mov %cr0, %ecx
	and $0xFFFB, %cx
	or $0x2, %cx
	mov %ecx, %cr0
	mov %cr4, %ecx
	or $0x600, %ecx
	mov %ecx, %cr4

	# Store a copy of the initialial floating point registers.
	fxsave fpu_initialized_regs

	# Enter the high-level kernel proper.
	subl $8, %esp # 16-byte align at call time.
	push %ebx # Multiboot information structure pointer.
	push %eax # Multiboot magic value.
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
