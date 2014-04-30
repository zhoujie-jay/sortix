/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2014.

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

    sockopt.cpp
    getsockopt() and setsockopt() utility functions.

*******************************************************************************/

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#include <sortix/kernel/ioctx.h>
#include <sortix/kernel/sockopt.h>

namespace Sortix {

bool sockopt_fetch_uintmax(uintmax_t* optval_ptr, ioctx_t* ctx,
                           const void* option_value, size_t option_size)
{
	uintmax_t optval;

	if ( option_size == 0 )
	{
		optval = 0;
	}
	else if ( option_size == 1 )
	{
		uint8_t truncated;
		if ( !ctx->copy_from_src(&truncated, option_value, sizeof(truncated)) )
			return false;
		optval = truncated;
	}
	else if ( option_size == 2 )
	{
		uint16_t truncated;
		if ( !ctx->copy_from_src(&truncated, option_value, sizeof(truncated)) )
			return false;
		optval = truncated;
	}
	else if ( option_size == 4 )
	{
		uint32_t truncated;
		if ( !ctx->copy_from_src(&truncated, option_value, sizeof(truncated)) )
			return false;
		optval = truncated;
	}
	else if ( option_size == 8 )
	{
		uint64_t truncated;
		if ( !ctx->copy_from_src(&truncated, option_value, sizeof(truncated)) )
			return false;
		optval = truncated;
	}
#if UINT64_MAX < UINTMAX_MAX
	#error "Add support for your large uintmax_t"
#endif
	else
	{
		return errno = EINVAL, false;
	}

	return *optval_ptr = optval, true;
}

bool sockopt_return_uintmax(uintmax_t optval, ioctx_t* ctx,
                            void* option_value, size_t* option_size_ptr)
{
	size_t option_size;
	if ( !ctx->copy_from_src(&option_size, option_size_ptr, sizeof(option_size)) )
		return false;

	if ( /*0 <= option_size &&*/ option_size < 1 )
	{
		option_size = 0;
	}
	else if ( 1 <= option_size && option_size < 2 )
	{
		if ( UINT8_MAX < optval )
			return errno = ERANGE, false;
		uint8_t value = optval;
		option_size = sizeof(value);
		if ( !ctx->copy_to_dest(option_value, &value, sizeof(value)) )
			return false;
	}
	else if ( 2 <= option_size && option_size < 4 )
	{
		if ( UINT16_MAX < optval )
			return errno = ERANGE, false;
		uint16_t value = optval;
		option_size = sizeof(value);
		if ( !ctx->copy_to_dest(option_value, &value, sizeof(value)) )
			return false;
	}
	else if ( 4 <= option_size && option_size < 8 )
	{
		if ( UINT32_MAX < optval )
			return errno = ERANGE, false;
		uint32_t value = optval;
		option_size = sizeof(value);
		if ( !ctx->copy_to_dest(option_value, &value, sizeof(value)) )
			return false;
	}
	else if ( 8 <= option_size /* && option_size < 16 */ )
	{
#if UINT64_MAX < UINTMAX_MAX
	#error "Add support for your large uintmax_t"
#endif
		uint64_t value = optval;
		option_size = sizeof(value);
		if ( !ctx->copy_to_dest(option_value, &value, sizeof(value)) )
			return false;
	}

	if ( !ctx->copy_to_dest(option_size_ptr, &option_size, sizeof(option_size)) )
		return false;

	return true;
}

} // namespace Sortix
