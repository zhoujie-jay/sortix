/*
 * Copyright (c) 2011, 2012, 2014 Jonas 'Sortie' Termansen.
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
 * sortix/kernel/sortedlist.h
 * A container that ensures its elements are always sorted when they are
 * accessed. It also provides binary search.
 */

#ifndef INCLUDE_SORTIX_KERNEL_SORTEDLIST_H
#define INCLUDE_SORTIX_KERNEL_SORTEDLIST_H

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

namespace Sortix {

template <class T> class SortedList
{
public:
	// GCC appears to be broken as the const in the function pointer typedef
	// below is ignored for some reason. Is it a compiler bug?
	typedef int (*compare_t)(const T, const T);

private:
	static const int FLAG_SORTED = 1 << 0;

private:
	T* list;
	size_t listused;
	size_t listlength;
	compare_t comparator;
	unsigned flags;

public:
	SortedList(compare_t thecomparator = NULL) :
		list(NULL),
		listused(0),
		listlength(0),
		comparator(thecomparator),
		flags(0)
	{

	}

	~SortedList()
	{
		Clear();
	}

public:
	void SetComparator(compare_t thecomparator)
	{
		comparator = thecomparator;
		flags &= ~FLAG_SORTED;
	}

	void Clear()
	{
		for ( size_t i = 0; i < listused; i++ )
			list[i].~T();

		listused = 0;
		flags |= FLAG_SORTED;
		delete[] list;
		list = NULL;
	}

	size_t Length()
	{
		return listused;
	}

	bool Empty()
	{
		return !listused;
	}

	bool IsSorted()
	{
		return flags & FLAG_SORTED;
	}

	void Sort(compare_t thecomparator)
	{
		comparator = thecomparator;
		Sort();
	}

	void Sort()
	{
		if ( !listused || (flags & FLAG_SORTED) )
			return;

		MergeSort(0, listused-1);

		flags |= FLAG_SORTED;
	}

	void Invalidate()
	{
		flags &= ~FLAG_SORTED;
	}

	bool Add(T t)
	{
		if ( listused == listlength && !Expand() )
			return false;

		list[listused++] = t;

		flags &= ~FLAG_SORTED;

		return true;
	}

	T Get(size_t index)
	{
		if ( !(flags & FLAG_SORTED) )
			Sort();

		return list[index];
	}

	// Accesses the elements, but the order might not be sorted.
	T GetUnsorted(size_t index)
	{
		return list[index];
	}

	T Remove(size_t index)
	{
		if ( !(flags & FLAG_SORTED) )
			Sort();
		assert(index < listused);

		// TODO: It may be possible to further speed up removal by delaying
		// the expensive memory copy operation.

		T result = list[index];
		list[index].~T();
		for ( size_t i = index+1; i < listused; i++ )
		{
			list[i-1] = list[i];
			list[i].~T();
		}

		listused--;

		return result;
	}

	size_t Search(T searchee)
	{
		return Search(comparator, searchee);
	}

	// Returns the index of the element being searched for using the given
	// comparator, or returns SIZE_MAX if not found.
	template <class Searchee>
	size_t Search(int (*searcher)(const T t, const Searchee searchee),
	              const Searchee searchee)
	{
		if ( !listused )
			return SIZE_MAX;
		if ( !(flags & FLAG_SORTED) )
			Sort();

		size_t minindex = 0;
		size_t maxindex = listused-1;

		do
		{
			size_t tryindex = (minindex + maxindex) / 2;
			const T& t = list[tryindex];
			int relation = searcher(t, searchee);
			if ( relation == 0 )
				return tryindex;
			if ( relation < 0 )
				minindex = tryindex+1;
			if ( relation > 0 )
				maxindex = tryindex-1;
		} while ( maxindex + 1 != minindex );

		return SIZE_MAX;
	}

private:
	bool Expand()
	{
		size_t newsize = (listused == 0 ) ? 4 : listlength*2;

		T* newlist = new T[newsize*2];
		if ( !newlist )
			return false;

		for ( size_t i = 0; i < listused; i++ )
		{
			newlist[i] = list[i];
			list[i].~T();
		}

		delete[] list;
		list = newlist;
		listlength = newsize;

		return true;
	}

	T* GetDestList()
	{
		return list + listlength;
	}

	void MergeSort(size_t minindex, size_t maxindex)
	{
		FixupMergeSort(minindex, maxindex);
	}

	void FixupMergeSort(size_t minindex, size_t maxindex)
	{
		MergeSortInternal(minindex, maxindex);

		T* destlist = GetDestList();

		do
			list[minindex] = destlist[minindex];
		while ( minindex++ != maxindex );
	}

	void MergeSortInternal(size_t minindex, size_t maxindex)
	{
		T* destlist = GetDestList();

		if ( maxindex - minindex == 1 )
		{
			if ( 0 < comparator(list[minindex], list[maxindex]) )
			{
				destlist[minindex] = list[maxindex];
				destlist[maxindex] = list[minindex];
				list[minindex].~T();
				list[maxindex].~T();
			}
			else
			{
				destlist[minindex] = list[minindex];
				destlist[maxindex] = list[maxindex];
				list[minindex].~T();
				list[maxindex].~T();
			}

			return;
		}

		if ( minindex == maxindex )
		{
			destlist[minindex] = list[minindex];
			list[minindex].~T();

			return;
		}

		const size_t asize = (maxindex - minindex + 1) / 2 + 1;
		const size_t bsize = (maxindex - minindex) / 2;
		const size_t astart = minindex;
		const size_t bstart = minindex + asize;
		const size_t aend = minindex + asize - 1;
		const size_t bend = minindex + asize + bsize - 1;

		FixupMergeSort(astart, aend);
		FixupMergeSort(bstart, bend);

		destlist += minindex;

		size_t aindex = astart;
		size_t bindex = bstart;

		while ( aindex <= aend && bindex <= bend )
		{
			int relation = comparator(list[aindex], list[bindex]);
			if ( relation <= 0 )
			{
				*destlist++ = list[aindex];
				list[aindex++].~T();
			}
			else
			{
				*destlist++ = list[bindex];
				list[bindex++].~T();
			}
		}

		while ( aindex <= aend )
		{
			*destlist++ = list[aindex];
			list[aindex++].~T();
		}

		while ( bindex <= bend )
		{
			*destlist++ = list[bindex];
			list[bindex++].~T();
		}
	}

};

} // namespace Sortix

#endif
