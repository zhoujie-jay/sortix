/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

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
.global isr130
.type isr130, @function
isr130:
	cli
	pushl $0 # err_code
	pushl $130 # int_no
	jmp interrupt_handler_prepare
.global isr131
.type isr131, @function
isr131:
	cli
	pushl $0 # err_code
	pushl $131 # int_no
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
.global yield_cpu_handler
.type yield_cpu_handler, @function
yield_cpu_handler:
	cli
	pushl $0 # err_code
	pushl $129 # int_no
	jmp interrupt_handler_prepare
.global thread_exit_handler
.type thread_exit_handler, @function
thread_exit_handler:
	cli
	pushl $0 # err_code
	pushl $132 # int_no
	jmp interrupt_handler_prepare

interrupt_handler_prepare:
	movl $1, asm_is_cpu_interrupted

	# Check if an interrupt happened while having kernel permissions.
	testw $0x3, 12(%esp) # cs
	jz fixup_relocate_stack
fixup_relocate_stack_complete:

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

	# Push the current kernel errno value.
	movl global_errno, %ebp
	pushl %ebp

	# Push whether a signal is pending.
	movl asm_signal_is_pending, %ebp
	pushl %ebp

	# Now call the interrupt handler.
	pushl %esp
	call interrupt_handler
	addl $4, %esp

load_interrupted_registers:
	# Restore whether signals are pending.
	popl %ebp
	movl %ebp, asm_signal_is_pending

	# Restore the previous kernel errno.
	popl %ebp
	movl %ebp, global_errno

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
	addl $4, %esp # Don't pop %esp, may not be defined.
	popl %ebx
	popl %edx
	popl %ecx
	popl %eax

	# Remove int_no and err_code
	addl $8, %esp

	movl $0, asm_is_cpu_interrupted

	# If interrupted with kernel permissions we may need to switch stack.
	testw $0x3, 4(%esp) # int_no and err_code now gone, so cs is at 4(%esp).
	jz fixup_switch_stack
fixup_switch_stack_complete:

	# Return to where we came from.
	iret

fixup_relocate_stack:
	# Ok, so some genius at Intel decided that if the permission level does not
	# change during an interrupt then the CPU won't push the stack pointer and
	# it won't reload it during iret. This seriously messes up the scheduler
	# that wants to preempt kernel threads each with their own stack. The
	# scheduler will attempt to read (and modify) the stack value which doesn't
	# exist and worse: the value at that location is likely used by the
	# interrupted kernel thread. A quick and dirty solution is to simply move
	# the stack 8 bytes down the stack. Right now there are the 5 elements on
	# the stack (eflags, cs, eip, err_code, int_no) of 5 bytes each.
	mov %eax, -4-8(%esp) # Save eax
	mov 0(%esp), %eax # int_no
	mov %eax, 0-8(%esp)
	mov 4(%esp), %eax # err_code
	mov %eax, 4-8(%esp)
	mov 8(%esp), %eax # eip
	mov %eax, 8-8(%esp)
	mov 12(%esp), %eax # cs
	mov %eax, 12-8(%esp)
	mov 16(%esp), %eax # eflags
	mov %eax, 16-8(%esp)
	# Next up we have to fake what the CPU should have done: pushed ss and esp.
	mov %esp, %eax
	addl $5*4, %eax # Calculate original esp
	mov %eax, 20-8(%esp)
	mov %ss, %eax
	mov %eax, 24-8(%esp)
	# Now that we moved the stack, it's time to really handle the interrupt.
	mov -4-8(%esp), %eax
	subl $8, %esp
	jmp fixup_relocate_stack_complete

fixup_switch_stack:
	# Yup, we also have to do special processing when we return from the
	# interrupt. The problem is that if the iret instruction won't load a new
	# stack if interrupted with kernel permissions and that the scheduler may
	# wish to change the current stack during a context switch. We will then
	# switch the stack before calling iret; but iret needs the return
	# information on the stack (and now it isn't), so we'll copy our stack onto
	# our new stack and then fire the interrupt and everyone is happy.

	# In the following code, %esp will point our fixed iret return parameters
	# that has stack data. However, the processor does not expect this
	# information as cs hasn't changed. %ebx will point to the new stack plus
	# room for three 32-bit values (eip, cs, eflags) that will be given to the
	# actual iret. We will then load the new stack and copy the eip, cs and
	# eflags to the new stack. However, we have to be careful in the case that
	# we are switching to the same stack (in which case stuff on the same
	# horizontal location in the diagram is actually on the same memory
	# location). We therefore copy to the new stack and carefully avoid
	# corrupting the destination if %esp + 8 = %ebx, This diagram show the
	# structure of the stacks and where temporaries will be stored:
	#      -12     -8      -4      %esp    4       8       12      16      20
	# old: IECX    IEBX    IEAX    EIP     CS      EFLAGS  ESP     SS      ...
	# new: IECX    IEBX    IEAX    -       -       EIP     CS      EFLAGS  ...
	#      -20     -16     -12     -8      -4      %ebx    4       8       12

	mov %eax, -4(%esp) # IEAX, Clobbered as copying temporary
	mov %ebx, -8(%esp) # IEBX, Clobbered as pointer to new stack
	mov %ecx, -12(%esp) # IECX, Clobbered as new stack selector

	mov 12(%esp), %ebx # Pointer to new stack
	sub $3*4, %ebx # Point to eip on the new stack (see diagram)
	movw 16(%esp), %cx # New ss

	# The order of these does not matter if we are switching to the same stack,
	# as the memory would be copied to the same location (see diagram).
	mov -4(%esp), %eax # interrupted eax value
	mov %eax, -12(%ebx)
	mov -8(%esp), %eax # interrupted ebx value
	mov %eax, -16(%ebx)
	mov -12(%esp), %eax # interrupted ecx value
	mov %eax, -20(%ebx)

	# The order of these three copies matter if switching to the same stack.
	mov 8(%esp), %eax # eflags
	mov %eax, 8(%ebx)
	mov 4(%esp), %eax # cs
	mov %eax, 4(%ebx)
	mov 0(%esp), %eax # eip
	mov %eax, 0(%ebx)

	mov %cx, %ss # Load new stack selector
	mov %ebx, %esp # Load new stack pointer

	mov -12(%esp), %eax # restore interrupted eax value
	mov -16(%esp), %ebx # restore interrupted ebx value
	mov -20(%esp), %ecx # restore interrupted ecx value

	jmp fixup_switch_stack_complete
.size interrupt_handler_prepare, . - interrupt_handler_prepare

.global interrupt_handler_null
.type interrupt_handler_null, @function
interrupt_handler_null:
	iret
.size interrupt_handler_null, . - interrupt_handler_null

.global asm_interrupts_are_enabled
.type asm_interrupts_are_enabled, @function
asm_interrupts_are_enabled:
	pushfl
	popl %eax
	andl $0x000200, %eax # FLAGS_INTERRUPT
	retl
.size asm_interrupts_are_enabled, . - asm_interrupts_are_enabled

.global load_registers
.type load_registers, @function
load_registers:
	# Let the register struct become our temporary stack
	movl 4(%esp), %esp
	jmp load_interrupted_registers
.size load_registers, . - load_registers
