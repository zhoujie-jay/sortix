/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011.

	This file is part of LibMaxsi.

	LibMaxsi is free software: you can redistribute it and/or modify it under
	the terms of the GNU Lesser General Public License as published by the Free
	Software Foundation, either version 3 of the License, or (at your option)
	any later version.

	LibMaxsi is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for
	more details.

	You should have received a copy of the GNU Lesser General Public License
	along with LibMaxsi. If not, see <http://www.gnu.org/licenses/>.

	c++.cpp
	Implements required C++ stuff for use in the Sortix kernel.

******************************************************************************/

#include <libmaxsi/platform.h>

extern "C" void __cxa_pure_virtual()
{
	// This shouldn't happen. TODO: Possibly crash the kernel here.
}

#ifdef PLATFORM_X86

extern "C" uint64_t __udivdi3(uint64_t a, uint64_t b)
{
	uint64_t result = 0;
	uint64_t power = 1;
	uint64_t remainder = a;
	uint64_t divisor = b;
	while ( divisor * 2 <= remainder )
	{
		power *= 2;
		divisor *= 2;
	}
	
	while ( divisor <= remainder )
	{
		remainder -= divisor;
		result += power;
		while ( power > 1 && remainder < divisor )
		{
			divisor /= 2;
			power /= 2;
		}
	}

	return result;
}

extern "C" uint64_t __umoddi3(uint64_t a, uint64_t b)
{
	uint64_t result = 0;
	uint64_t power = 1;
	uint64_t remainder = a;
	uint64_t divisor = b;
	while ( divisor * 2 <= remainder )
	{
		power *= 2;
		divisor *= 2;
	}
	
	while ( divisor <= remainder )
	{
		remainder -= divisor;
		result += power;
		while ( power > 1 && remainder < divisor )
		{
			divisor /= 2;
			power /= 2;
		}
	}

	return remainder;
}

extern "C" int64_t __divdi3(int64_t a, int64_t b)
{
	if ( a >= 0 && b >= 0 )
	{
		uint64_t numer = a;
		uint64_t denom = b;
		uint64_t result = __udivdi3(numer, denom);
		return +((int64_t) result);
	}
	else if ( a < 0 && b >= 0 )
	{
		uint64_t numer = -a;
		uint64_t denom = b;
		uint64_t result = __udivdi3(numer, denom);
		return -((int64_t) result);
	}
	else if ( a >= 0 && b < 0 )
	{
		uint64_t numer = a;
		uint64_t denom = -b;
		uint64_t result = __udivdi3(numer, denom);
		return -((int64_t) result);
	}
	else // if ( a < 0 && b < 0 )
	{
		uint64_t numer = -a;
		uint64_t denom = -b;
		uint64_t result = __udivdi3(numer, denom);
		return +((int64_t) result);
	}
}

extern "C" int64_t __moddi3(int64_t a, int64_t b)
{
	if ( a >= 0 && b >= 0 )
	{
		uint64_t numer = a;
		uint64_t denom = b;
		uint64_t result = __umoddi3(numer, denom);
		return +((int64_t) result);
	}
	else if ( a < 0 && b >= 0 )
	{
		uint64_t numer = -a;
		uint64_t denom = b;
		uint64_t result = __umoddi3(numer, denom);
		return -((int64_t) result);
	}
	else if ( a >= 0 && b < 0 )
	{
		uint64_t numer = a;
		uint64_t denom = -b;
		uint64_t result = __umoddi3(numer, denom);
		return +((int64_t) result);
	}
	else // if ( a < 0 && b < 0 )
	{
		uint64_t numer = -a;
		uint64_t denom = -b;
		uint64_t result = __umoddi3(numer, denom);
		return -((int64_t) result);
	}
}

#endif

