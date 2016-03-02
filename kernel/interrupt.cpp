/*
 * Copyright (c) 2011, 2012, 2013, 2014 Jonas 'Sortie' Termansen.
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
 * interrupt.cpp
 * High level interrupt services.
 */

#include <assert.h>
#include <errno.h>
#include <string.h>

#include <sortix/kernel/cpu.h>
#include <sortix/kernel/interrupt.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/process.h>
#include <sortix/kernel/scheduler.h>
#include <sortix/kernel/signal.h>
#include <sortix/kernel/syscall.h>
#include <sortix/kernel/thread.h>

// TODO: The interrupt worker isn't a reliable design.

namespace Sortix {
namespace Interrupt {

unsigned char* queue;
volatile size_t queue_offset;
volatile size_t queue_used;
const size_t QUEUE_SIZE = 4096;

struct worker_package
{
	size_t payload_size;
	void (*handler)(void*, void*, size_t);
	void* handler_context;
};

void InitWorker()
{
	queue = new unsigned char[QUEUE_SIZE];
	if ( !queue )
		Panic("Can't allocate interrupt worker queue");
	queue_offset = 0;
	queue_used = 0;
}

static void WriteToQueue(const void* src, size_t size)
{
	assert(size <= QUEUE_SIZE - queue_used);
	const unsigned char* input = (const unsigned char*) src;
	for ( size_t i = 0; i < size; i++ )
	{
		size_t index = (queue_offset + queue_used + i) % QUEUE_SIZE;
		queue[index] = input[i];
	}
	queue_used += size;
}

static void ReadFromQueue(void* dst, size_t size)
{
	assert(size <= queue_used);
	unsigned char* output = (unsigned char*) dst;
	for ( size_t i = 0; i < size; i++ )
	{
		size_t index = (queue_offset + i) % QUEUE_SIZE;
		output[i] = queue[index];
	}
	queue_offset = (queue_offset + size) % QUEUE_SIZE;
	queue_used -= size;
}

static bool PopPackage(struct worker_package* package,
                       unsigned char* payload,
                       size_t payload_size)
{
	bool interrupts_was_enabled = Interrupt::SetEnabled(false);
	if ( !queue_used )
	{
		Interrupt::SetEnabled(interrupts_was_enabled);
		return false;
	}

	ReadFromQueue(package, sizeof(*package));
	if ( !(package->payload_size <= payload_size) )
	{
		Interrupt::SetEnabled(interrupts_was_enabled);
		assert(package->payload_size <= payload_size);
		queue_offset = (queue_offset + package->payload_size) % QUEUE_SIZE;
		return false;
	}

	ReadFromQueue(payload, package->payload_size);

	Interrupt::SetEnabled(interrupts_was_enabled);
	return true;
}

void WorkerThread(void* /*user*/)
{
	assert(Interrupt::IsEnabled());

	struct worker_package package;
	size_t storage_size = QUEUE_SIZE;
	unsigned char* storage = new unsigned char[storage_size];
	if ( !storage )
		Panic("Can't allocate interrupt worker storage");
	while ( true )
	{
		if ( !PopPackage(&package, storage, storage_size) )
		{
			Scheduler::Yield();
			continue;
		}
		unsigned char* payload = storage;
		size_t payload_size = package.payload_size;
		assert(package.handler);
		package.handler(package.handler_context, payload, payload_size);
	}
}

bool ScheduleWork(void (*handler)(void*, void*, size_t),
                  void* handler_context,
                  void* payload,
                  size_t payload_size)
{
	assert(!Interrupt::IsEnabled());

	assert(handler);
	assert(payload || !payload_size);

	struct worker_package package;
	memset(&package, 0, sizeof(package));
	package.payload_size = payload_size;
	package.handler = handler;
	package.handler_context = handler_context;

	if ( QUEUE_SIZE - queue_used < sizeof(package) + payload_size )
		return false;

	WriteToQueue(&package, sizeof(package));
	WriteToQueue(payload, payload_size);

	return true;
}

} // namespace Interrupt
} // namespace Sortix
