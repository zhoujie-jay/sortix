/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012, 2013.

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

    sortix/kernel/refcount.h
    A class that implements reference counting.

*******************************************************************************/

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
		if ( obj ) { obj->Unref_Renamed(); obj = NULL; }
		if ( (obj = r.Get()) ) obj->Refer_Renamed();
		return *this;
	}

	template <class U>
	Ref operator=(const Ref<U> r)
	{
		if ( obj ) { obj->Unref_Renamed(); obj = NULL; }
		if ( (obj = r.Get()) ) obj->Refer_Renamed();
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
