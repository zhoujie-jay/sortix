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

#include <sortix/kernel/calltrace.h>
#include <sortix/kernel/cpu.h>
#include <sortix/kernel/debugger.h>
#include <sortix/kernel/interrupt.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/process.h>
#include <sortix/kernel/scheduler.h>
#include <sortix/kernel/signal.h>
#include <sortix/kernel/syscall.h>
#include <sortix/kernel/thread.h>

namespace Sortix {
namespace Interrupt {

// TODO: This implementation is a bit hacky and can be optimized.

uint8_t* queue;
uint8_t* storage;
volatile size_t queueoffset;
volatile size_t queueused;
size_t queuesize;

struct Package
{
	size_t size;
	size_t payloadoffset;
	size_t payloadsize;
	WorkHandler handler;
	uint8_t payload[0];
};

void InitWorker()
{
	const size_t QUEUE_SIZE = 4UL*1024UL;
	static_assert(QUEUE_SIZE % sizeof(Package) == 0, "QUEUE_SIZE must be a multiple of the package size");
	queue = new uint8_t[QUEUE_SIZE];
	if ( !queue )
		Panic("Can't allocate interrupt worker queue");
	storage = new uint8_t[QUEUE_SIZE];
	if ( !storage )
		Panic("Can't allocate interrupt worker storage");
	queuesize = QUEUE_SIZE;
	queueoffset = 0;
	queueused = 0;
}

static void WriteToQueue(const void* src, size_t size)
{
	const uint8_t* buf = (const uint8_t*) src;
	size_t writeat = (queueoffset + queueused) % queuesize;
	size_t available = queuesize - writeat;
	size_t count = available < size ? available : size;
	memcpy(queue + writeat, buf, count);
	queueused += count;
	if ( count < size )
		WriteToQueue(buf + count, size - count);
}

static void ReadFromQueue(void* dest, size_t size)
{
	uint8_t* buf = (uint8_t*) dest;
	size_t available = queuesize - queueoffset;
	size_t count = available < size ? available : size;
	memcpy(buf, queue + queueoffset, count);
	queueused -= count;
	queueoffset = (queueoffset + count) % queuesize;
	if ( count < size )
		ReadFromQueue(buf + count, size - count);
}

static Package* PopPackage(uint8_t** payloadp, Package* /*prev*/)
{
	Package* package = NULL;
	uint8_t* payload = NULL;
	Interrupt::Disable();

	if ( !queueused )
		goto out;

	package = (Package*) storage;
	ReadFromQueue(package, sizeof(*package));
	payload = storage + sizeof(*package);
	ReadFromQueue(payload, package->payloadsize);
	*payloadp = payload;

out:
	Interrupt::Enable();
	return package;
}

void WorkerThread(void* /*user*/)
{
	assert(Interrupt::IsEnabled());
	uint8_t* payload = NULL;
	Package* package = NULL;
	while ( true )
	{
		package = PopPackage(&payload, package);
		if ( !package ) { Scheduler::Yield(); continue; }
		size_t payloadsize = package->payloadsize;
		package->handler(payload, payloadsize);
	}
}

bool ScheduleWork(WorkHandler handler, void* payload, size_t payloadsize)
{
	assert(!Interrupt::IsEnabled());

	Package package;
	package.size = sizeof(package) + payloadsize;
	package.payloadoffset = 0; // Currently unused
	package.payloadsize = payloadsize;
	package.handler = handler;

	size_t queuefreespace = queuesize - queueused;
	if ( queuefreespace < package.size )
		return false;

	WriteToQueue(&package, sizeof(package));
	WriteToQueue(payload, payloadsize);
	return true;
}

} // namespace Interrupt
} // namespace Sortix
