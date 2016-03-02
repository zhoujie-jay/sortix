/*
 * Copyright (c) 2012, 2013, 2015 Jonas 'Sortie' Termansen.
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
 * textbuffer.cpp
 * Provides a indexable text buffer for used by text mode terminals.
 */

#include <assert.h>
#include <errno.h>

#include <sortix/kernel/kernel.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/refcount.h>
#include <sortix/kernel/textbuffer.h>

namespace Sortix {

TextBufferHandle::TextBufferHandle(TextBuffer* textbuf)
{
	this->textbuf = textbuf;
	this->numused = 0;
	this->mutex = KTHREAD_MUTEX_INITIALIZER;
	this->unusedcond = KTHREAD_COND_INITIALIZER;
}

TextBufferHandle::~TextBufferHandle()
{
	delete textbuf;
}

TextBuffer* TextBufferHandle::Acquire()
{
	ScopedLock lock(&mutex);
	if ( !textbuf )
		return errno = EINIT, (TextBuffer*) NULL;
	numused++;
	return textbuf;
}

void TextBufferHandle::Release(TextBuffer* textbuf)
{
	assert(textbuf);
	ScopedLock lock(&mutex);
	assert(numused);
	numused--;
	if ( numused == 0 )
		kthread_cond_signal(&unusedcond);
}

bool TextBufferHandle::EmergencyIsImpaired()
{
	if ( !kthread_mutex_trylock(&mutex) )
		return true;
	kthread_mutex_unlock(&mutex);
	return false;
}

bool TextBufferHandle::EmergencyRecoup()
{
	if ( !EmergencyIsImpaired() )
		return true;
	mutex = KTHREAD_MUTEX_INITIALIZER;
	return true;
}

void TextBufferHandle::EmergencyReset()
{
}

TextBuffer* TextBufferHandle::EmergencyAcquire()
{
	// This is during a kernel emergency where preemption has been disabled and
	// this is the only thread running.
	return textbuf;
}

void TextBufferHandle::EmergencyRelease(TextBuffer* textbuf)
{
	// This is during a kernel emergency where preemption has been disabled and
	// this is the only thread running. We don't maintain the reference count
	// during this state, so this is a no-operation.
	(void) textbuf;
}

void TextBufferHandle::Replace(TextBuffer* newtextbuf)
{
	ScopedLock lock(&mutex);
	while ( 0 < numused )
		kthread_cond_wait(&unusedcond, &mutex);
	delete textbuf;
	textbuf = newtextbuf;
}

} // namespace Sortix
