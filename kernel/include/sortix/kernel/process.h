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

    sortix/kernel/process.h
    A named collection of threads.

*******************************************************************************/

#ifndef INCLUDE_SORTIX_KERNEL_PROCESS_H
#define INCLUDE_SORTIX_KERNEL_PROCESS_H

#include <sortix/fork.h>
#include <sortix/resource.h>
#include <sortix/sigaction.h>
#include <sortix/signal.h>
#include <sortix/sigset.h>

#include <sortix/kernel/clock.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/refcount.h>
#include <sortix/kernel/registers.h>
#include <sortix/kernel/segment.h>
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
class ProcessTable;
struct ProcessSegment;
struct ProcessTimer;
struct ioctx_struct;
typedef struct ioctx_struct ioctx_t;
struct segment;
struct Symbol;

class Process
{
friend void Process__OnLastThreadExit(void*);

public:
	Process();
	~Process();

public:
	char* string_table;
	size_t string_table_length;
	Symbol* symbol_table;
	size_t symbol_table_length;
	char* program_image_path;
	addr_t addrspace;
	pid_t pid;

public:
	kthread_mutex_t nicelock;
	int nice;

public:
	kthread_mutex_t idlock;
	uid_t uid, euid;
	gid_t gid, egid;
	mode_t umask;

private:
	kthread_mutex_t ptrlock;
	Ref<Descriptor> root;
	Ref<Descriptor> cwd;
	Ref<MountTable> mtable;
	Ref<DescriptorTable> dtable;

public:
	Ref<ProcessTable> ptable;

public:
	kthread_mutex_t resource_limits_lock;
	struct rlimit resource_limits[RLIMIT_NUM_DECLARED];

public:
	kthread_mutex_t signal_lock;
	struct sigaction signal_actions[SIG_MAX_NUM];
	sigset_t signal_pending;
	void (*sigreturn)(void);

public:
	void BootstrapTables(Ref<DescriptorTable> dtable, Ref<MountTable> mtable);
	void BootstrapDirectories(Ref<Descriptor> root);
	Ref<DescriptorTable> GetDTable();
	Ref<MountTable> GetMTable();
	Ref<ProcessTable> GetPTable();
	Ref<Descriptor> GetRoot();
	Ref<Descriptor> GetCWD();
	Ref<Descriptor> GetDescriptor(int fd);
	void SetRoot(Ref<Descriptor> newroot);
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
	int exit_code;

public:
	Process* group;
	Process* groupprev;
	Process* groupnext;
	Process* groupfirst;
	kthread_mutex_t groupchildlock;
	kthread_mutex_t groupparentlock;
	kthread_cond_t groupchildleft;
	bool grouplimbo;

public:
	Thread* firstthread;
	kthread_mutex_t threadlock;
	bool threads_exiting;

public:
	struct segment* segments;
	size_t segments_used;
	size_t segments_length;
	kthread_mutex_t segment_write_lock;
	kthread_mutex_t segment_lock;

public:
	kthread_mutex_t user_timers_lock;
	UserTimer user_timers[PROCESS_TIMER_NUM_MAX];
	Timer alarm_timer;
	Clock execute_clock;
	Clock system_clock;
	Clock child_execute_clock;
	Clock child_system_clock;

public:
	int Execute(const char* programname, const uint8_t* program,
	            size_t programsize, int argc, const char* const* argv,
	            int envc, const char* const* envp,
	            struct thread_registers* regs);
	void ResetAddressSpace();
	void ExitThroughSignal(int signal);
	void ExitWithCode(int exit_code);
	pid_t Wait(pid_t pid, int* status, int options);
	bool DeliverSignal(int signum);
	bool DeliverGroupSignal(int signum);
	void OnThreadDestruction(Thread* thread);
	pid_t GetParentProcessId();
	void AddChildProcess(Process* child);
	void ScheduleDeath();
	void AbortConstruction();
	bool MapSegment(struct segment* result, void* hint, size_t size, int flags,
	                int prot);

public:
	Process* Fork();

private:
	void OnLastThreadExit();
	void LastPrayer();
	void NotifyMemberExit(Process* child);
	void NotifyChildExit(Process* child, bool zombify);
	void NotifyNewZombies();
	void DeleteTimers();

public:
	void NotifyLeftProcessGroup();

public:
	void ResetForExecute();

};

Process* CurrentProcess();

} // namespace Sortix

#endif
