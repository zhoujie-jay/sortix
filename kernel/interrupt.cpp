/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2014.

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

    interrupt.cpp
    High level interrupt services.

*******************************************************************************/

#include <assert.h>
#include <errno.h>
#include <string.h>

#include <sortix/kernel/cpu.h>
#include <sortix/kernel/debugger.h>
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
