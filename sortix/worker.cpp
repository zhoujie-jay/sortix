/*******************************************************************************

	Copyright(C) Jonas 'Sortie' Termansen 2012.

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

	worker.cpp
	Kernel worker thread.

*******************************************************************************/

#include <sortix/kernel/platform.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/worker.h>

namespace Sortix {
namespace Worker {

struct Job
{
	void (*func)(void*);
	void* user;
};

static kthread_mutex_t jobslock;
static kthread_cond_t jobsready;
static kthread_cond_t jobsfree;
static size_t jobsused;
static size_t jobsoff;
static size_t jobslen;
static Job* jobs;

void Init()
{
	jobslock = KTHREAD_MUTEX_INITIALIZER;
	jobsready = KTHREAD_COND_INITIALIZER;
	jobsfree = KTHREAD_COND_INITIALIZER;
	const size_t NUM_JOBS = 128UL;
	jobslen = NUM_JOBS;
	jobs = new Job[jobslen];
	if ( !jobs )
		Panic("Unable to allocate worker thread job queue");
	jobsused = 0;
	jobsoff = 0;
}

static bool _Schedule(void (*func)(void*), void* user, bool retry)
{
	ScopedLock lock(&jobslock);
	if ( jobsused == jobslen && !retry )
		return false;
	while ( jobsused == jobslen )
		kthread_cond_wait(&jobsfree, &jobslock);
	if ( !jobsused )
		kthread_cond_signal(&jobsready);
	size_t index = (jobsoff + jobsused++) % jobslen;
	jobs[index].func = func;
	jobs[index].user = user;
	return true;
}

void Schedule(void (*func)(void*), void* user)
{
	_Schedule(func, user, true);
}

bool TrySchedule(void (*func)(void*), void* user)
{
	return _Schedule(func, user, false);
}

static void PerformJob(Job* job)
{
	job->func(job->user);
}

void Thread(void* /*user*/)
{
	while ( true )
	{
		kthread_mutex_lock(&jobslock);
		while ( !jobsused )
			kthread_cond_wait(&jobsready, &jobslock);
		Job job = jobs[jobsoff];
		jobsoff = (jobsoff+1) % jobslen;
		if ( jobsused-- == jobslen )
			kthread_cond_signal(&jobsfree);
		kthread_mutex_unlock(&jobslock);
		PerformJob(&job);
	}
}

} // namespace Worker
} // namespace Sortix
