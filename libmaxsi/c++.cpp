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

#include "platform.h"

extern "C" void __cxa_pure_virtual()
{
	// This shouldn't happen. TODO: Possibly crash the kernel here.
}

#ifdef PLATFORM_X86

// TODO: These two stubs are buggy!

// Divides two 64-bit integers in O(logN). I think.
extern "C" uint64_t __udivdi3(uint64_t A, uint64_t B)
{
	uint64_t ACC = 0;
	uint64_t R = 0;
	uint64_t PWR = 1;
	uint64_t EXP = B;

	while ( EXP <= A - ACC )
	{
		R += PWR;
		ACC += EXP;
		
		if ( 2 * EXP <= A - ACC )
		{
			PWR++;
			EXP *= 2;
		}
		else if ( A - ACC < B )
		{
			break;
		}
		else
		{
			while ( A - ACC < EXP )
			{
				PWR--;
				EXP /= 2;
			}
		}
	}

	return R;
}

// Mods two 64-bit integers in O(logN). I think.
extern "C" uint64_t __umoddi3(uint64_t A, uint64_t B)
{
	uint64_t ACC = 0;
	uint64_t R = 0;
	uint64_t PWR = 1;
	uint64_t EXP = B;

	while ( EXP <= A - ACC )
	{
		R += PWR;
		ACC += EXP;
		
		if ( 2 * EXP <= A - ACC )
		{
			PWR++;
			EXP *= 2;
		}
		else if ( A - ACC < B )
		{
			break;
		}
		else
		{
			while ( A - ACC < EXP )
			{
				PWR--;
				EXP /= 2;
			}
		}
	}

	return A - ACC;
}
#endif

