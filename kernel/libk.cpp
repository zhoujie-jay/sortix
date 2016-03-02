/*
 * Copyright (c) 2016 Jonas 'Sortie' Termansen.
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
 * libk.cpp
 * Hooks for libk.
 */

#include <libk.h>

#include <sortix/mman.h>

#include <sortix/kernel/panic.h>

#include <sortix/kernel/addralloc.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/memorymanagement.h>
#include <sortix/kernel/random.h>

namespace Sortix {

static kthread_mutex_t heap_mutex = KTHREAD_MUTEX_INITIALIZER;
static kthread_mutex_t random_mutex = KTHREAD_MUTEX_INITIALIZER;

extern "C"
void libk_assert(const char* filename,
                 unsigned long line,
                 const char* function_name,
                 const char* expression)
{
	Sortix::PanicF("Assertion failure: %s:%lu: %s: %s", filename, line,
	               function_name, expression);
}

extern "C"
size_t libk_getpagesize(void)
{
	return Sortix::Page::Size();
}

extern "C"
void* libk_heap_expand(size_t* num_bytes)
{
	// Decide where we would like to add memory to the heap.
	uintptr_t mapto = Sortix::GetHeapUpper();
	void* mapping = (void*) mapto;

	// Attempt to allocate the needed virtual address space such that we can put
	// memory there to extend the heap with.
	if ( !(*num_bytes = Sortix::ExpandHeap(*num_bytes)) )
		return NULL;

	// Attempt to map actual memory at our new virtual addresses.
	int prot = PROT_KREAD | PROT_KWRITE;
	enum Sortix::page_usage page_usage = Sortix::PAGE_USAGE_KERNEL_HEAP;
	if ( !Sortix::Memory::MapRange(mapto, *num_bytes, prot, page_usage) )
		return NULL;
	Sortix::Memory::Flush();

	return mapping;
}

extern "C"
void libk_heap_lock(void)
{
	kthread_mutex_lock(&heap_mutex);
}

extern "C"
void libk_heap_unlock(void)
{
	kthread_mutex_unlock(&heap_mutex);
}

extern "C"
void libk_stack_chk_fail(void)
{
	Panic("Stack smashing detected");
}

extern "C"
void libk_abort(void)
{
	Sortix::PanicF("abort()");
}

extern "C"
void libk_random_lock(void)
{
	kthread_mutex_lock(&random_mutex);
}

extern "C"
void libk_random_unlock(void)
{
	kthread_mutex_unlock(&random_mutex);
}

extern "C"
bool libk_hasentropy(void)
{
	return Sortix::Random::HasEntropy();
}

extern "C"
void libk_getentropy(void* buffer, size_t size)
{
	Sortix::Random::GetEntropy(buffer, size);
}

extern "C"
__attribute__((noreturn))
void libk_ubsan_abort(const char* violation,
                      const char* filename,
                      uint32_t line,
                      uint32_t column)
{
	Sortix::PanicF("Undefined behavior: %s at %s:%u:%u",
		violation, filename, line, column);
}

extern "C"
void* libk_mmap(size_t size, int prot)
{
	size = Page::AlignUp(size);
	addralloc_t addralloc;
	if ( !AllocateKernelAddress(&addralloc, size) )
		return NULL;
	if ( !Memory::MapRange(addralloc.from, size, prot, PAGE_USAGE_KERNEL_HEAP) )
	{
		Memory::Flush();
		FreeKernelAddress(&addralloc);
		return NULL;
	}
	Memory::Flush();
	return (void*) addralloc.from;
}

extern "C"
void libk_mprotect(void* ptr, size_t size, int prot)
{
	addr_t mapto = (addr_t) ptr;
	for ( size_t i = 0; i < size; i += Page::Size() )
		Memory::PageProtect(mapto + i, prot);
	Memory::Flush();
}

extern "C"
void libk_munmap(void* ptr, size_t size)
{
	size = Page::AlignUp(size);
	addralloc_t addralloc;
	addralloc.from = (addr_t) ptr;
	addralloc.size = size;
	Memory::UnmapRange(addralloc.from, size, PAGE_USAGE_KERNEL_HEAP);
	Memory::Flush();
	FreeKernelAddress(&addralloc);
}

} // namespace Sortix
