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

	sortix-sound.cpp
	Provides access to the sound devices. This is highly unportable and is very
	likely to be removed or changed radically.

******************************************************************************/

#include <libmaxsi/platform.h>
#include <libmaxsi/sortix-sound.h>
#include <sys/syscall.h>

namespace System
{
	namespace Sound
	{
		DEFN_SYSCALL1_VOID(SysSetFrequency, SYSCALL_SET_FREQUENCY, unsigned);

		void SetFrequency(unsigned frequency)
		{
			SysSetFrequency(frequency);
		}
	}
}

