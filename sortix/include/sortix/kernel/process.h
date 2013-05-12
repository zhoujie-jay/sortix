/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013.

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

    sortix/kernel/process.h
    A named collection of threads.

*******************************************************************************/

#ifndef INCLUDE_SORTIX_KERNEL_PROCESS_H
#define INCLUDE_SORTIX_KERNEL_PROCESS_H

#include <sortix/fork.h>

#include <sortix/kernel/clock.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/refcount.h>
#include <sortix/kernel/time.h>
#include <sortix/kernel/timer.h>
#include <sortix/kernel/user-timer.h>
#include <sortix/kernel/cpu.h>

#define PROCESS_TIMER_NUM_MAX 32

namespace Sortix {

class Thread;
class Process;
class Descriptor;
class DescriptorTable;
class MountTable;
struct ProcessSegment;
struct ProcessTimer;
struct ioctx_struct;
typedef struct ioctx_struct ioctx_t;

const int SEG_NONE = 0;
const int SEG_TEXT = 1;
const int SEG_DATA = 2;
const int SEG_STACK = 3;
const int SEG_OTHER = 4;

struct ProcessSegment
{
public:
	ProcessSegment() : prev(NULL), next(NULL) { }

public:
	ProcessSegment* prev;
	ProcessSegment* next;
	addr_t position;
	size_t size;
	int type;

public:
	bool Intersects(ProcessSegment* segments);
	ProcessSegment* Fork();

};

class Process
{
friend void Process__OnLastThreadExit(void*);

public:
	Process();
	~Process();

public:
	static void Init();

private:
	static pid_t AllocatePID();

public:
	char* program_image_path;
	addr_t addrspace;
	pid_t pid;

public:
	kthread_mutex_t idlock;
	uid_t uid, euid;
	gid_t gid, egid;

private:
	kthread_mutex_t ptrlock;
	Ref<Descriptor> root;
	Ref<Descriptor> cwd;
	Ref<MountTable> mtable;
	Ref<DescriptorTable> dtable;

public:
	void BootstrapTables(Ref<DescriptorTable> dtable, Ref<MountTable> mtable);
	void BootstrapDirectories(Ref<Descriptor> root);
	Ref<MountTable> GetMTable();
	Ref<DescriptorTable> GetDTable();
	Ref<Descriptor> GetRoot();
	Ref<Descriptor> GetCWD();
	Ref<Descriptor> GetDescriptor(int fd);
	// TODO: This should be removed, don't call it.
	Ref<Descriptor> Open(ioctx_t* ctx, const char* path, int flags, mode_t mode = 0);
	void SetCWD(Ref<Descriptor> newcwd);

private:
// A process may only access its parent if parentlock is locked. A process
// may only use its list of children if childlock is locked. A process may
// not access its sibling processes.
	Process* parent;
	Process* prevsibling;
	Process* nextsibling;
	Process* firstchild;
	Process* zombiechild;
	kthread_mutex_t childlock;
	kthread_mutex_t parentlock;
	kthread_cond_t zombiecond;
	size_t zombiewaiting;
	bool iszombie;
	bool nozombify;
	addr_t mmapfrom;
	int exitstatus;

public:
	Thread* firstthread;
	kthread_mutex_t threadlock;

public:
	ProcessSegment* segments;

public:
	kthread_mutex_t user_timers_lock;
	UserTimer user_timers[PROCESS_TIMER_NUM_MAX];
	Timer alarm_timer;

public:
	int Execute(const char* programname, const uint8_t* program,
	            size_t programsize, int argc, const char* const* argv,
	            int envc, const char* const* envp,
	            CPU::InterruptRegisters* regs);
	void ResetAddressSpace();
	void Exit(int status);
	pid_t Wait(pid_t pid, int* status, int options);
	bool DeliverSignal(int signum);
	void OnThreadDestruction(Thread* thread);
	int GetParentProcessId();
	void AddChildProcess(Process* child);
	void ScheduleDeath();
	void AbortConstruction();

public:
	Process* Fork();

private:
	void ExecuteCPU(int argc, char** argv, int envc, char** envp,
	                addr_t stackpos, addr_t entry,
	                CPU::InterruptRegisters* regs);
	void OnLastThreadExit();
	void LastPrayer();
	void NotifyChildExit(Process* child, bool zombify);
	void NotifyNewZombies();
	void DeleteTimers();

public:
	void ResetForExecute();
	addr_t AllocVirtualAddr(size_t size);

public:
	static Process* Get(pid_t pid);
	static pid_t HackGetForegroundProcess();

private:
	static bool Put(Process* process);
	static void Remove(Process* process);

};

void InitializeThreadRegisters(CPU::InterruptRegisters* regs,
                               const tforkregs_t* requested);
Process* CurrentProcess();

} // namespace Sortix

#endif
