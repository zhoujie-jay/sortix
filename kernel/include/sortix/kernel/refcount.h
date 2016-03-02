/*
 * Copyright (c) 2012, 2013 Jonas 'Sortie' Termansen.
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
 * sortix/kernel/refcount.h
 * A class that implements reference counting.
 */

#ifndef SORTIX_REFCOUNT_H
#define SORTIX_REFCOUNT_H

#include <sortix/kernel/kthread.h>

namespace Sortix {

class Refcountable
{
public:
	Refcountable();
	virtual ~Refcountable();

public:
	void Refer_Renamed();
	void Unref_Renamed();
	size_t Refcount() const { return refcount; }
	bool IsUnique() const { return refcount == 1; }

private:
	kthread_mutex_t reflock;
	size_t refcount;

public:
	bool being_deleted;

};

template <class T> class Ref
{
public:
	constexpr Ref() : obj(NULL) { }
	explicit Ref(T* obj) : obj(obj) { if ( obj ) obj->Refer_Renamed(); }
	template <class U>
	explicit Ref(U* obj) : obj(obj) { if ( obj ) obj->Refer_Renamed(); }
	Ref(const Ref<T>& r) : obj(r.Get()) { if ( obj ) obj->Refer_Renamed(); }
	template <class U>
	Ref(const Ref<U>& r) : obj(r.Get()) { if ( obj ) obj->Refer_Renamed(); }
	~Ref() { if ( obj ) obj->Unref_Renamed(); }

	Ref& operator=(const Ref r)
	{
		if ( obj == r.Get() )
			return *this;
		if ( obj )
		{
			obj->Unref_Renamed();
			obj = NULL;
		}
		if ( (obj = r.Get()) )
			obj->Refer_Renamed();
		return *this;
	}

	template <class U>
	Ref operator=(const Ref<U> r)
	{
		if ( obj == r.Get() )
			return *this;
		if ( obj )
		{
			obj->Unref_Renamed();
			obj = NULL;
		}
		if ( (obj = r.Get()) )
			obj->Refer_Renamed();
		return *this;
	}

	bool operator==(const Ref& other)
	{
		return (*this).Get() == other.Get();
	}

	template <class U> bool operator==(const Ref<U>& other)
	{
		return (*this).Get() == other.Get();
	}

	template <class U> bool operator==(const U* const& other)
	{
		return (*this).Get() == other;
	}

	bool operator!=(const Ref& other)
	{
		return !((*this) == other);
	}

	template <class U> bool operator!=(const Ref<U>& other)
	{
		return !((*this) == other);
	}

	template <class U> bool operator!=(const U* const& other)
	{
		return !((*this) == other);
	}

	void Reset() { if ( obj ) obj->Unref_Renamed(); obj = NULL; }
	T* Get() const { return obj; }
	T& operator *() const { return *obj; }
	T* operator->() const { return obj; }
	operator bool() const { return obj != NULL; }
	size_t Refcount() const { return obj ? obj->Refcount : 0; }
	bool IsUnique() const { return obj->IsUnique(); }

private:
	T* obj;

};

} // namespace Sortix

#endif
