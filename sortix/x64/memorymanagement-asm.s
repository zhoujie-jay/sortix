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

	memorymanagement-asm.s
	Handles memory for the x64 architecture.

******************************************************************************/

.section .text

.globl _ZN6Sortix4Page11FragmentizeEv
.type _ZN6Sortix4Page11FragmentizeEv, @function # namespace Sortix { namespace Page { void Fragmentize(); } }
_ZN6Sortix4Page11FragmentizeEv:
	# Load the front of our linked list.
	movq _ZN6Sortix4Page15UnallocatedPageE, %rax # Load UnallocPage* Sortix::Page::UnallocatedPage

FragLoop:
	testq %rax, %rax
	je FragDone

	movq 8(%rax), %rdx
	movq 16(%rax), %rbx

	testq %rbx, %rbx
	je FragNext

	movq %rax, %rcx
	addq $0x1000, %rcx
	movq %rcx, 8(%rax)
	decq %rbx

FragFixupLoop:
	testq %rbx, %rbx
	je FragFixupLoopLastSwap

	movl $0xABBAACDC, 0(%rcx)
	addq $0x2000, %rax
	movq %rax, 8(%rcx)
	#movq $0, 16(%rcx)
	decq %rbx

	testq %rbx, %rbx
	je FragFixupLoopLast

	movl $0xABBAACDC, 0(%rax)
	addq $0x2000, %rcx
	movq %rcx, 8(%rax)
	#movq $0, 16(%rax)
	decq %rbx

	jmp FragFixupLoop

FragFixupLoopLastSwap:
	movq %rcx, %rax

FragFixupLoopLast:
	movl $0xABBAACDC, 0(%rax)
	movq %rdx, 8(%rax)

FragNext:
	movq %rdx, %rax
	jmp FragLoop

FragDone:
	ret	

.globl _ZN6Sortix4Page10GetPrivateEv
.type _ZN6Sortix4Page10GetPrivateEv, @function # namespace Sortix { void Paging::Allocate(); }
_ZN6Sortix4Page10GetPrivateEv:
	# Load the front of our linked list.
	movq _ZN6Sortix4Page15UnallocatedPageE, %rax # Load UnallocPage* Sortix::Page::UnallocatedPage

	# Test if we are out of memory.
	testq %rax, %rax
	je OutOfMem

	push %rbx
	push %rdi
	push %rsi

	# Disable virtual memory
	movq %cr0, %rdi
	movq %rdi, %rsi
	movabs $0xffffffff7fffffff, %rbx
	andq %rbx, %rsi 
	movq %rsi, %cr0

	# Test if this page is unallocated, as a security measure.	
	movl (%rax), %ebx
	cmpl $0xABBAACDC, %ebx
	jne MagicFailure

	# Prepare the next front element in our linked list.
	movq 8(%rax), %rdx
	jmp Done

MagicFailure:
	# The magic header in our page was invalid!
	movq %rax, %rdx
	inc %rax # Set the lowest bit to signal a magic failure.

	# Panic the kernel.
	mov %rdx, %rsi
	mov %rbx, %rdx
	mov MagicFailureString, %rdi
	call PanicF

Done:
	# Enable virtual memory here
	mov %rdi, %cr0

	# Update the front of our linked list to the next element.	
	mov %rdx, _ZN6Sortix4Page15UnallocatedPageE # Save Load UnallocPage* Sortix::Page::UnallocatedPage

	ret

OutOfMem:
	# We run out of memory. Return NULL.
	movq $0, %rax
	ret

.globl _ZN6Sortix4Page10PutPrivateEm
.type _ZN6Sortix4Page10PutPrivateEm, @function # namespace Sortix { void Paging::Free(void* Page); }
_ZN6Sortix4Page10PutPrivateEm:
	movq _ZN6Sortix4Page15UnallocatedPageE, %rax # Load UnallocPage* Sortix::Page::UnallocatedPage

	# Disable virtual memory
	movq %cr0, %rcx
	movq %rcx, %rsi
	movabs $0xffffffff7fffffff, %rbx
	andq %rbx, %rsi 
	movq %rsi, %cr0

	movl $0xABBAACDC, (%rdi)
	movq %rax, 8(%edx)
	#movl $0, 16(%edx)

	# Enable virtual memory
	movq %rcx, %cr0

	# Update the front of our linked list to the next element.	
	movq %rdx, _ZN6Sortix4Page15UnallocatedPageE # Save Load UnallocPage* Sortix::Page::UnallocatedPage
	ret

.section .data
MagicFailureString:
	.asciz "A magic failure occured at 0x%p, where 0x%x was found instead of 0xABBAACDC!\n"
