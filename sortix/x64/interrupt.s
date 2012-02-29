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

	x63/interrupt.s
	Transfers control to interrupt handlers when interrupts happen.

******************************************************************************/

.section .text

.global isr0
.type isr0, @function
isr0:
	cli
	pushq $0 # err_code
	pushq $0 # int_no
	jmp interrupt_handler_prepare
.global isr1
.type isr1, @function
isr1:
	cli
	pushq $0 # err_code
	pushq $1 # int_no
	jmp interrupt_handler_prepare
.global isr2
.type isr2, @function
isr2:
	cli
	pushq $0 # err_code
	pushq $2 # int_no
	jmp interrupt_handler_prepare
.global isr3
.type isr3, @function
isr3:
	cli
	pushq $0 # err_code
	pushq $3 # int_no
	jmp interrupt_handler_prepare
.global isr4
.type isr4, @function
isr4:
	cli
	pushq $0 # err_code
	pushq $4 # int_no
	jmp interrupt_handler_prepare
.global isr5
.type isr5, @function
isr5:
	cli
	pushq $0 # err_code
	pushq $5 # int_no
	jmp interrupt_handler_prepare
.global isr6
.type isr6, @function
isr6:
	cli
	pushq $0 # err_code
	pushq $6 # int_no
	jmp interrupt_handler_prepare
.global isr7
.type isr7, @function
isr7:
	cli
	pushq $0 # err_code
	pushq $7 # int_no
	jmp interrupt_handler_prepare
.global isr8
.type isr8, @function
isr8:
	cli
	# pushq $0 # err_code pushed by CPU
	pushq $8 # int_no
	jmp interrupt_handler_prepare
.global isr9
.type isr9, @function
isr9:
	cli
	pushq $0 # err_code
	pushq $9 # int_no
	jmp interrupt_handler_prepare
.global isr10
.type isr10, @function
isr10:
	cli
	# pushq $0 # err_code pushed by CPU
	pushq $10 # int_no
	jmp interrupt_handler_prepare
.global isr11
.type isr11, @function
isr11:
	cli
	# pushq $0 # err_code pushed by CPU
	pushq $11 # int_no
	jmp interrupt_handler_prepare
.global isr12
.type isr12, @function
isr12:
	cli
	# pushq $0 # err_code pushed by CPU
	pushq $12 # int_no
	jmp interrupt_handler_prepare
.global isr13
.type isr13, @function
isr13:
	cli
	# pushq $0 # err_code pushed by CPU
	pushq $13 # int_no
	jmp interrupt_handler_prepare
.global isr14
.type isr14, @function
isr14:
	cli
	# pushq $0 # err_code pushed by CPU
	pushq $14 # int_no
	jmp interrupt_handler_prepare
.global isr15
.type isr15, @function
isr15:
	cli
	pushq $0 # err_code
	pushq $15 # int_no
	jmp interrupt_handler_prepare
.global isr16
.type isr16, @function
isr16:
	cli
	pushq $0 # err_code
	pushq $16 # int_no
	jmp interrupt_handler_prepare
.global isr17
.type isr17, @function
isr17:
	cli
	pushq $0 # err_code
	pushq $17 # int_no
	jmp interrupt_handler_prepare
.global isr18
.type isr18, @function
isr18:
	cli
	pushq $0 # err_code
	pushq $18 # int_no
	jmp interrupt_handler_prepare
.global isr19
.type isr19, @function
isr19:
	cli
	pushq $0 # err_code
	pushq $19 # int_no
	jmp interrupt_handler_prepare
.global isr20
.type isr20, @function
isr20:
	cli
	pushq $0 # err_code
	pushq $20 # int_no
	jmp interrupt_handler_prepare
.global isr21
.type isr21, @function
isr21:
	cli
	pushq $0 # err_code
	pushq $21 # int_no
	jmp interrupt_handler_prepare
.global isr22
.type isr22, @function
isr22:
	cli
	pushq $0 # err_code
	pushq $22 # int_no
	jmp interrupt_handler_prepare
.global isr23
.type isr23, @function
isr23:
	cli
	pushq $0 # err_code
	pushq $23 # int_no
	jmp interrupt_handler_prepare
.global isr24
.type isr24, @function
isr24:
	cli
	pushq $0 # err_code
	pushq $24 # int_no
	jmp interrupt_handler_prepare
.global isr25
.type isr25, @function
isr25:
	cli
	pushq $0 # err_code
	pushq $25 # int_no
	jmp interrupt_handler_prepare
.global isr26
.type isr26, @function
isr26:
	cli
	pushq $0 # err_code
	pushq $26 # int_no
	jmp interrupt_handler_prepare
.global isr27
.type isr27, @function
isr27:
	cli
	pushq $0 # err_code
	pushq $27 # int_no
	jmp interrupt_handler_prepare
.global isr28
.type isr28, @function
isr28:
	cli
	pushq $0 # err_code
	pushq $28 # int_no
	jmp interrupt_handler_prepare
.global isr29
.type isr29, @function
isr29:
	cli
	pushq $0 # err_code
	pushq $29 # int_no
	jmp interrupt_handler_prepare
.global isr30
.type isr30, @function
isr30:
	cli
	pushq $0 # err_code
	pushq $30 # int_no
	jmp interrupt_handler_prepare
.global isr31
.type isr31, @function
isr31:
	cli
	pushq $0 # err_code
	pushq $31 # int_no
	jmp interrupt_handler_prepare
.global isr128
.type isr128, @function
isr128:
	cli
	pushq $0 # err_code
	pushq $128 # int_no
	jmp interrupt_handler_prepare
.global irq0
.type irq0, @function
irq0:
	cli
	pushq $0 # err_code
	pushq $32 # int_no
	jmp interrupt_handler_prepare
.global irq1
.type irq1, @function
irq1:
	cli
	pushq $0 # err_code
	pushq $33 # int_no
	jmp interrupt_handler_prepare
.global irq2
.type irq2, @function
irq2:
	cli
	pushq $0 # err_code
	pushq $34 # int_no
	jmp interrupt_handler_prepare
.global irq3
.type irq3, @function
irq3:
	cli
	pushq $0 # err_code
	pushq $35 # int_no
	jmp interrupt_handler_prepare
.global irq4
.type irq4, @function
irq4:
	cli
	pushq $0 # err_code
	pushq $36 # int_no
	jmp interrupt_handler_prepare
.global irq5
.type irq5, @function
irq5:
	cli
	pushq $0 # err_code
	pushq $37 # int_no
	jmp interrupt_handler_prepare
.global irq6
.type irq6, @function
irq6:
	cli
	pushq $0 # err_code
	pushq $38 # int_no
	jmp interrupt_handler_prepare
.global irq7
.type irq7, @function
irq7:
	cli
	pushq $0 # err_code
	pushq $39 # int_no
	jmp interrupt_handler_prepare
.global irq8
.type irq8, @function
irq8:
	cli
	pushq $0 # err_code
	pushq $40 # int_no
	jmp interrupt_handler_prepare
.global irq9
.type irq9, @function
irq9:
	cli
	pushq $0 # err_code
	pushq $41 # int_no
	jmp interrupt_handler_prepare
.global irq10
.type irq10, @function
irq10:
	cli
	pushq $0 # err_code
	pushq $42 # int_no
	jmp interrupt_handler_prepare
.global irq11
.type irq11, @function
irq11:
	cli
	pushq $0 # err_code
	pushq $43 # int_no
	jmp interrupt_handler_prepare
.global irq12
.type irq12, @function
irq12:
	cli
	pushq $0 # err_code
	pushq $44 # int_no
	jmp interrupt_handler_prepare
.global irq13
.type irq13, @function
irq13:
	cli
	pushq $0 # err_code
	pushq $45 # int_no
	jmp interrupt_handler_prepare
.global irq14
.type irq14, @function
irq14:
	cli
	pushq $0 # err_code
	pushq $46 # int_no
	jmp interrupt_handler_prepare
.global irq15
.type irq15, @function
irq15:
	cli
	pushq $0 # err_code
	pushq $47 # int_no
	jmp interrupt_handler_prepare

interrupt_handler_prepare:
	pushq %r15
	pushq %r14
	pushq %r13
	pushq %r12
	pushq %r11
	pushq %r10
	pushq %r9
	pushq %r8
	pushq %rax
	pushq %rcx
	pushq %rdx
	pushq %rbx
	pushq %rsp
	pushq %rbp
	pushq %rsi
	pushq %rdi

	# Push the user-space data segment.
	movl %ds, %ebp
	pushq %rbp

	# Load the kernel data segment.
	movw $0x10, %bp
	movl %ebp, %ds
	movl %ebp, %es
	movl %ebp, %fs
	movl %ebp, %gs

	# Push CR2 in case of page faults
	movq %cr2, %rbp
	pushq %rbp

	# Now call the interrupt handler.
	movq %rsp, %rdi
	call interrupt_handler

	# Remove CR2 from the stack.
	addq $8, %rsp

	# Restore the user-space data segment.
	popq %rbp
	movl %ebp, %ds
	movl %ebp, %es
	movl %ebp, %fs
	movl %ebp, %gs

	popq %rdi
	popq %rsi
	popq %rbp
	popq %rsp
	popq %rbx
	popq %rdx
	popq %rcx
	popq %rax
	popq %r8
	popq %r9
	popq %r10
	popq %r11
	popq %r12
	popq %r13
	popq %r14
	popq %r15

	# Remove int_no and err_code
	addq $16, %rsp

	# Return to where we came from.
	iretq

.global interrupt_handler_null
.type interrupt_handler_null, @function
interrupt_handler_null:
	iretq

