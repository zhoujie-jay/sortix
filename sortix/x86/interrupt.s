/*******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011, 2012.

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

	x86/interrupt.s
	Transfers control to interrupt handlers when interrupts happen.

*******************************************************************************/

.section .text

.global isr0
.type isr0, @function
isr0:
	cli
	pushl $0 # err_code
	pushl $0 # int_no
	jmp interrupt_handler_prepare
.global isr1
.type isr1, @function
isr1:
	cli
	pushl $0 # err_code
	pushl $1 # int_no
	jmp interrupt_handler_prepare
.global isr2
.type isr2, @function
isr2:
	cli
	pushl $0 # err_code
	pushl $2 # int_no
	jmp interrupt_handler_prepare
.global isr3
.type isr3, @function
isr3:
	cli
	pushl $0 # err_code
	pushl $3 # int_no
	jmp interrupt_handler_prepare
.global isr4
.type isr4, @function
isr4:
	cli
	pushl $0 # err_code
	pushl $4 # int_no
	jmp interrupt_handler_prepare
.global isr5
.type isr5, @function
isr5:
	cli
	pushl $0 # err_code
	pushl $5 # int_no
	jmp interrupt_handler_prepare
.global isr6
.type isr6, @function
isr6:
	cli
	pushl $0 # err_code
	pushl $6 # int_no
	jmp interrupt_handler_prepare
.global isr7
.type isr7, @function
isr7:
	cli
	pushl $0 # err_code
	pushl $7 # int_no
	jmp interrupt_handler_prepare
.global isr8
.type isr8, @function
isr8:
	cli
	# pushl $0 # err_code pushed by CPU
	pushl $8 # int_no
	jmp interrupt_handler_prepare
.global isr9
.type isr9, @function
isr9:
	cli
	pushl $0 # err_code
	pushl $9 # int_no
	jmp interrupt_handler_prepare
.global isr10
.type isr10, @function
isr10:
	cli
	# pushl $0 # err_code pushed by CPU
	pushl $10 # int_no
	jmp interrupt_handler_prepare
.global isr11
.type isr11, @function
isr11:
	cli
	# pushl $0 # err_code pushed by CPU
	pushl $11 # int_no
	jmp interrupt_handler_prepare
.global isr12
.type isr12, @function
isr12:
	cli
	# pushl $0 # err_code pushed by CPU
	pushl $12 # int_no
	jmp interrupt_handler_prepare
.global isr13
.type isr13, @function
isr13:
	cli
	# pushl $0 # err_code pushed by CPU
	pushl $13 # int_no
	jmp interrupt_handler_prepare
.global isr14
.type isr14, @function
isr14:
	cli
	# pushl $0 # err_code pushed by CPU
	pushl $14 # int_no
	jmp interrupt_handler_prepare
.global isr15
.type isr15, @function
isr15:
	cli
	pushl $0 # err_code
	pushl $15 # int_no
	jmp interrupt_handler_prepare
.global isr16
.type isr16, @function
isr16:
	cli
	pushl $0 # err_code
	pushl $16 # int_no
	jmp interrupt_handler_prepare
.global isr17
.type isr17, @function
isr17:
	cli
	pushl $0 # err_code
	pushl $17 # int_no
	jmp interrupt_handler_prepare
.global isr18
.type isr18, @function
isr18:
	cli
	pushl $0 # err_code
	pushl $18 # int_no
	jmp interrupt_handler_prepare
.global isr19
.type isr19, @function
isr19:
	cli
	pushl $0 # err_code
	pushl $19 # int_no
	jmp interrupt_handler_prepare
.global isr20
.type isr20, @function
isr20:
	cli
	pushl $0 # err_code
	pushl $20 # int_no
	jmp interrupt_handler_prepare
.global isr21
.type isr21, @function
isr21:
	cli
	pushl $0 # err_code
	pushl $21 # int_no
	jmp interrupt_handler_prepare
.global isr22
.type isr22, @function
isr22:
	cli
	pushl $0 # err_code
	pushl $22 # int_no
	jmp interrupt_handler_prepare
.global isr23
.type isr23, @function
isr23:
	cli
	pushl $0 # err_code
	pushl $23 # int_no
	jmp interrupt_handler_prepare
.global isr24
.type isr24, @function
isr24:
	cli
	pushl $0 # err_code
	pushl $24 # int_no
	jmp interrupt_handler_prepare
.global isr25
.type isr25, @function
isr25:
	cli
	pushl $0 # err_code
	pushl $25 # int_no
	jmp interrupt_handler_prepare
.global isr26
.type isr26, @function
isr26:
	cli
	pushl $0 # err_code
	pushl $26 # int_no
	jmp interrupt_handler_prepare
.global isr27
.type isr27, @function
isr27:
	cli
	pushl $0 # err_code
	pushl $27 # int_no
	jmp interrupt_handler_prepare
.global isr28
.type isr28, @function
isr28:
	cli
	pushl $0 # err_code
	pushl $28 # int_no
	jmp interrupt_handler_prepare
.global isr29
.type isr29, @function
isr29:
	cli
	pushl $0 # err_code
	pushl $29 # int_no
	jmp interrupt_handler_prepare
.global isr30
.type isr30, @function
isr30:
	cli
	pushl $0 # err_code
	pushl $30 # int_no
	jmp interrupt_handler_prepare
.global isr31
.type isr31, @function
isr31:
	cli
	pushl $0 # err_code
	pushl $31 # int_no
	jmp interrupt_handler_prepare
.global isr128
.type isr128, @function
isr128:
	cli
	pushl $0 # err_code
	pushl $128 # int_no
	jmp interrupt_handler_prepare
.global irq0
.type irq0, @function
irq0:
	cli
	pushl $0 # err_code
	pushl $32 # int_no
	jmp interrupt_handler_prepare
.global irq1
.type irq1, @function
irq1:
	cli
	pushl $0 # err_code
	pushl $33 # int_no
	jmp interrupt_handler_prepare
.global irq2
.type irq2, @function
irq2:
	cli
	pushl $0 # err_code
	pushl $34 # int_no
	jmp interrupt_handler_prepare
.global irq3
.type irq3, @function
irq3:
	cli
	pushl $0 # err_code
	pushl $35 # int_no
	jmp interrupt_handler_prepare
.global irq4
.type irq4, @function
irq4:
	cli
	pushl $0 # err_code
	pushl $36 # int_no
	jmp interrupt_handler_prepare
.global irq5
.type irq5, @function
irq5:
	cli
	pushl $0 # err_code
	pushl $37 # int_no
	jmp interrupt_handler_prepare
.global irq6
.type irq6, @function
irq6:
	cli
	pushl $0 # err_code
	pushl $38 # int_no
	jmp interrupt_handler_prepare
.global irq7
.type irq7, @function
irq7:
	cli
	pushl $0 # err_code
	pushl $39 # int_no
	jmp interrupt_handler_prepare
.global irq8
.type irq8, @function
irq8:
	cli
	pushl $0 # err_code
	pushl $40 # int_no
	jmp interrupt_handler_prepare
.global irq9
.type irq9, @function
irq9:
	cli
	pushl $0 # err_code
	pushl $41 # int_no
	jmp interrupt_handler_prepare
.global irq10
.type irq10, @function
irq10:
	cli
	pushl $0 # err_code
	pushl $42 # int_no
	jmp interrupt_handler_prepare
.global irq11
.type irq11, @function
irq11:
	cli
	pushl $0 # err_code
	pushl $43 # int_no
	jmp interrupt_handler_prepare
.global irq12
.type irq12, @function
irq12:
	cli
	pushl $0 # err_code
	pushl $44 # int_no
	jmp interrupt_handler_prepare
.global irq13
.type irq13, @function
irq13:
	cli
	pushl $0 # err_code
	pushl $45 # int_no
	jmp interrupt_handler_prepare
.global irq14
.type irq14, @function
irq14:
	cli
	pushl $0 # err_code
	pushl $46 # int_no
	jmp interrupt_handler_prepare
.global irq15
.type irq15, @function
irq15:
	cli
	pushl $0 # err_code
	pushl $47 # int_no
	jmp interrupt_handler_prepare

interrupt_handler_prepare:
	pushl %eax
	pushl %ecx
	pushl %edx
	pushl %ebx
	pushl %esp
	pushl %ebp
	pushl %esi
	pushl %edi

	# Push the user-space data segment.
	movl %ds, %ebp
	pushl %ebp

	# Load the kernel data segment.
	movw $0x10, %bp
	movl %ebp, %ds
	movl %ebp, %es
	movl %ebp, %fs
	movl %ebp, %gs

	# Push CR2 in case of page faults
	movl %cr2, %ebp
	pushl %ebp

	# Now call the interrupt handler.
	pushl %esp
	call interrupt_handler
	addl $4, %esp

	# Remove CR2 from the stack.
	addl $4, %esp

	# Restore the user-space data segment.
	popl %ebp
	movl %ebp, %ds
	movl %ebp, %es
	movl %ebp, %fs
	movl %ebp, %gs

	popl %edi
	popl %esi
	popl %ebp
	popl %esp
	popl %ebx
	popl %edx
	popl %ecx
	popl %eax

	# Remove int_no and err_code
	addl $8, %esp

	# Return to where we came from.
	iret

.global interrupt_handler_null
.type interrupt_handler_null, @function
interrupt_handler_null:
	iret

.global asm_interrupts_are_enabled
.type asm_interrupts_are_enabled, @function
asm_interrupts_are_enabled:
	pushfl
	popl %eax
	andl $0x000200, %eax # FLAGS_INTERRUPT
	retl

