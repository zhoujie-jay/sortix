/*
 * Copyright (c) 2012 Jonas 'Sortie' Termansen.
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
 * worker.cpp
 * Kernel worker thread.
 */

#include <sortix/kernel/kernel.h>
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
