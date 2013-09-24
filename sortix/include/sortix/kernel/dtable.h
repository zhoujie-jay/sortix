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

    sortix/kernel/dtable.h
    Table of file descriptors.

*******************************************************************************/

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
	~DescriptorTable();
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

private:
	void Reset(); // Hey, reference counted. Don't call this.
	bool IsGoodEntry(int i);
	bool Enlargen(int atleast);
	int AllocateInternal(Ref<Descriptor> desc, int flags, int min_index);

private:
	kthread_mutex_t dtablelock;
	dtableent_t* entries;
	int numentries;

};

} // namespace Sortix

#endif
