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
	Handles memory for the x86 architecture.

******************************************************************************/

.section .text

.globl _ZN6Sortix4Page11FragmentizeEv
.type _ZN6Sortix4Page11FragmentizeEv, @function # namespace Sortix { namespace Page { void Fragmentize(); } }
_ZN6Sortix4Page11FragmentizeEv:
	push %ebx

	# Load the front of our linked list.
	mov _ZN6Sortix4Page15UnallocatedPageE, %eax # Load UnallocPage* Sortix::Page::UnallocatedPage

FragLoop:
	testl %eax, %eax
	je FragDone

	movl 4(%eax), %edx
	movl 8(%eax), %ebx

	testl %ebx, %ebx
	je FragNext

	movl %eax, %ecx
	addl $0x1000, %ecx
	movl %ecx, 4(%eax)
	decl %ebx

FragFixupLoop:
	testl %ebx, %ebx
	je FragFixupLoopLastSwap

	movl $0xABBAACDC, 0(%ecx)
	addl $0x2000, %eax
	movl %eax, 4(%ecx)
	#movl $0, 8(%ecx)
	decl %ebx

	testl %ebx, %ebx
	je FragFixupLoopLast

	movl $0xABBAACDC, 0(%eax)
	addl $0x2000, %ecx
	movl %ecx, 4(%eax)
	#movl $0, 8(%eax)
	decl %ebx

	jmp FragFixupLoop

FragFixupLoopLastSwap:
	movl %ecx, %eax

FragFixupLoopLast:
	movl $0xABBAACDC, 0(%eax)
	movl %edx, 4(%eax)

FragNext:
	movl %edx, %eax
	jmp FragLoop

FragDone:
	pop %ebx
	ret	

.globl _ZN6Sortix4Page10GetPrivateEv
.type _ZN6Sortix4Page10GetPrivateEv, @function # namespace Sortix { void Paging::Allocate(); }
_ZN6Sortix4Page10GetPrivateEv:
	# Load the front of our linked list.
	mov _ZN6Sortix4Page15UnallocatedPageE, %eax # Load UnallocPage* Sortix::Page::UnallocatedPage

	# Test if we are out of memory.
	testl %eax, %eax
	je OutOfMem

	push %ebx
	push %edi
	push %esi

	# Disable virtual memory
	mov %cr0, %edi
	mov %edi, %esi
	and $0x7FFFFFFF, %esi 
	mov %esi, %cr0

	# Test if this page is unallocated, as a security measure.	
	movl (%eax), %ebx
	cmpl $0xABBAACDC, %ebx
	jne MagicFailure

	# Prepare the next front element in our linked list.
	movl 4(%eax), %edx
	jmp Done

MagicFailure:
	# The magic header in our page was invalid!
	movl %eax, %edx
	inc %eax # Set the lowest bit to signal a magic failure.

	# Panic the kernel.
	pushl %ebx
	pushl %edx
	pushl MagicFailureString
	call PanicF

Done:
	# Enable virtual memory here
	mov %edi, %cr0

	# Update the front of our linked list to the next element.	
	mov %edx, _ZN6Sortix4Page15UnallocatedPageE # Save Load UnallocPage* Sortix::Page::UnallocatedPage

	pop %esi
	pop %edi
	pop %ebx

	ret

OutOfMem:
	# We run out of memory. Return NULL.
	movl $0, %eax
	ret

.globl _ZN6Sortix4Page10PutPrivateEm
.type _ZN6Sortix4Page10PutPrivateEm, @function # namespace Sortix { void Paging::Free(void* Page); }
_ZN6Sortix4Page10PutPrivateEm:
	push %esi
	mov _ZN6Sortix4Page15UnallocatedPageE, %eax # Load UnallocPage* Sortix::Page::UnallocatedPage
	mov 0x8(%esp), %edx

	# Disable virtual memory
	mov %cr0, %ecx
	mov %ecx, %esi
	and $0x7FFFFFFF, %esi
	mov %esi, %cr0

	movl $0xABBAACDC, (%edx)
	movl %eax, 4(%edx)
	#movl $0, 8(%edx)

	# Enable virtual memory
	mov %ecx, %cr0

	# Update the front of our linked list to the next element.	
	mov %edx, _ZN6Sortix4Page15UnallocatedPageE # Save Load UnallocPage* Sortix::Page::UnallocatedPage
	pop %esi
	ret

.section .data
MagicFailureString:
	.asciz "Test!"
	.asciz "Test2!\0"
	.asciz "A magic failure occured at 0x%p, where 0x%x was found instead of 0xABBAACDC!\n"
