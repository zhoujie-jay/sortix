/*
 * Copyright (c) 2013 Jonas 'Sortie' Termansen.
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
 * sortix/kernel/yielder.h
 * Template that allows creation of easily-iterable sequences.
 */

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
