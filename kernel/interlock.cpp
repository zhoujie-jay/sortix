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
 * interlock.cpp
 * Functions that perform non-atomic operations in an atomic manner.
 */

#include <sortix/kernel/interlock.h>
#include <sortix/kernel/kernel.h>

namespace Sortix {

// TODO: This is likely not the most optimal way to perform these operations.

ilret_t InterlockedModify(unsigned long* ptr,
                          ilockfunc f,
                          unsigned long user)
{
	unsigned long old_value, new_value;
	do
	{
		old_value = *((volatile unsigned long*) ptr); /* TODO: Need volatile? */
		new_value = f(old_value, user);
	} while ( !__sync_bool_compare_and_swap(ptr, old_value, new_value) );
	ilret_t ret;
	ret.o = old_value;
	ret.n = new_value;
	return ret;
}

static unsigned long AddFunction(unsigned long val, unsigned long arg)
{
	return val + arg;
}

static unsigned long SubFunction(unsigned long val, unsigned long arg)
{
	return val - arg;
}

ilret_t InterlockedIncrement(unsigned long* ptr)
{
	return InterlockedModify(ptr, AddFunction, 1);
}

ilret_t InterlockedDecrement(unsigned long* ptr)
{
	return InterlockedModify(ptr, SubFunction, 1);
}

ilret_t InterlockedAdd(unsigned long* ptr, unsigned long arg)
{
	return InterlockedModify(ptr, AddFunction, arg);
}

ilret_t InterlockedSub(unsigned long* ptr, unsigned long arg)
{
	return InterlockedModify(ptr, SubFunction, arg);
}

} // namespace Sortix
