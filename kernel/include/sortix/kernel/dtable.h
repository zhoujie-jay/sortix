/*
 * Copyright (c) 2011, 2012, 2013, 2014, 2015 Jonas 'Sortie' Termansen.
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
 * sortix/kernel/dtable.h
 * Table of file descriptors.
 */

#ifndef SORTIX_DTABLE_H
#define SORTIX_DTABLE_H

#include <sortix/kernel/refcount.h>

namespace Sortix {

class Descriptor;

typedef struct dtableent_struct
{
	Ref<Descriptor> desc;
	int flags;
} dtableent_t;

class DescriptorTable : public Refcountable
{
public:
	DescriptorTable();
	virtual ~DescriptorTable();
	Ref<DescriptorTable> Fork();
	Ref<Descriptor> Get(int index);
	int Allocate(Ref<Descriptor> desc, int flags, int min_index = 0);
	int Allocate(int src_index, int flags, int min_index = 0);
	int Copy(int from, int to, int flags);
	void Free(int index);
	Ref<Descriptor> FreeKeep(int index);
	void OnExecute();
	bool SetFlags(int index, int flags);
	int GetFlags(int index);
	int Previous(int index);
	int Next(int index);
	int CloseFrom(int index);

private:
	void Reset(); // Hey, reference counted. Don't call this.
	bool IsGoodEntry(int i);
	bool Enlargen(int atleast);
	int AllocateInternal(Ref<Descriptor> desc, int flags, int min_index);
	Ref<Descriptor> FreeKeepInternal(int index);

private:
	kthread_mutex_t dtablelock;
	dtableent_t* entries;
	int numentries;
	int first_not_taken;

};

} // namespace Sortix

#endif
