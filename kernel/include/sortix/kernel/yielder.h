/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

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

    sortix/kernel/yielder.h
    Template that allows creation of easily-iterable sequences.

*******************************************************************************/

#ifndef INCLUDE_SORTIX_KERNEL_YIELDER_H
#define INCLUDE_SORTIX_KERNEL_YIELDER_H

namespace Sortix {

class finished_yielder { };

template <class yielder_type, class yielded_type> class yielder_iterator
{
public:
	yielder_iterator(yielder_type yielder) : yielder_object(yielder)
	{
		has_value = yielder_object.yield(&current_value);
	}

	bool is_finished() const
	{
		return !has_value;
	}

	bool operator!=(const yielder_iterator& other) const
	{
		return !(is_finished() && other.is_finished());
	}

	yielded_type operator*()
	{
		return current_value;
	}

	const yielder_iterator& operator++()
	{
		has_value = yielder_object.yield(&current_value);
		return *this;
	}

private:
	yielder_type yielder_object;
	yielded_type current_value;
	bool has_value;

};

} // namespace Sortix

#endif
