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

	textbuffer.cpp
	Provides a indexable text buffer for used by text mode terminals.

*******************************************************************************/

#include <sortix/kernel/platform.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/refcount.h>
#include <sortix/kernel/textbuffer.h>
#include <assert.h>

namespace Sortix {

TextBufferHandle::TextBufferHandle(TextBuffer* textbuf, bool deletebuf,
                                   TextBuffer* def, bool deletedef)
{
	this->textbuf = textbuf;
	this->deletebuf = deletebuf;
	this->def = def;
	this->deletedef = deletedef;
	this->numused = 0;
	this->mutex = KTHREAD_MUTEX_INITIALIZER;
	this->unusedcond = KTHREAD_COND_INITIALIZER;
}

TextBufferHandle::~TextBufferHandle()
{
	if ( deletebuf )
		delete textbuf;
	if ( deletedef )
		delete def;
}

TextBuffer* TextBufferHandle::Acquire()
{
	ScopedLock lock(&mutex);
	numused++;
	if ( textbuf )
		return textbuf;
	if ( !def )
		numused--;
	return def;
}

void TextBufferHandle::Release(TextBuffer* textbuf)
{
	assert(textbuf);
	ScopedLock lock(&mutex);
	assert(numused);
	if ( !--numused )
		kthread_cond_signal(&unusedcond);
}

void TextBufferHandle::Replace(TextBuffer* newtextbuf, bool deletebuf)
{
	ScopedLock lock(&mutex);
	while ( numused )
		kthread_cond_wait(&unusedcond, &mutex);
	if ( deletebuf )
		delete textbuf;
	this->textbuf = newtextbuf;
	this->deletebuf = deletebuf;
}

} // namespace Sortix
