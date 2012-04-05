/*******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2012.

	This file is part of LibMaxsi.

	LibMaxsi is free software: you can redistribute it and/or modify it under
	the terms of the GNU Lesser General Public License as published by the Free
	Software Foundation, either version 3 of the License, or (at your option)
	any later version.

	LibMaxsi is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
	details.

	You should have received a copy of the GNU Lesser General Public License
	along with LibMaxsi. If not, see <http://www.gnu.org/licenses/>.

	x86/fork.s
	Assembly functions related to forking x86 processes.

*******************************************************************************/

.section .text

.globl __call_sforkr_with_regs
.type __call_sforkr_with_regs, @function
__call_sforkr_with_regs:
	pushl %ebp
	movl %esp, %ebp

	movl 8(%ebp), %edx # flags parameter, edx need not be preserved.

	# The actual system call expects a struct sforkregs_x86 containing the state
	# of each register in the child. Since we create an identical copy, we
	# simply set each member of the structure to our own state. Note that since
	# the stack goes downwards, we create it in the reverse order.
	pushfl
	pushl %ebp
	pushl %esp
	pushl %esi
	pushl %edi
	pushl %edx
	pushl %ecx
	pushl %ebx
	pushl $0 # rax, result of sfork is 0 for the child.
	pushl $after_fork # rip, child will start execution from here.

	# Call sforkr with a nice pointer to our structure. Note that %edi contains
	# the flag parameter that this function accepted.
	pushl %esp
	pushl %edx
	call sforkr

after_fork:
	# The value in %eax determines whether we are child or parent. There is no
	# need to clean up the stack from the above pushes, leavel sets %esp to %ebp
	# which does that for us.
	leavel
	retl

