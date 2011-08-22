/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011.

	This file is part of Sortix.

	Sortix is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License as published by the Free Software
	Foundation, either version 3 of the License, or (at your option) any later
	version.

	Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
	details.

	You should have received a copy of the GNU General Public License along
	with Sortix. If not, see <http://www.gnu.org/licenses/>.

	sound.h
	Implements a simple sound system.

******************************************************************************/

#include "platform.h"
#include "sound.h"

namespace Sortix
{
	namespace Sound
	{
		void Mute()
		{
		 	uint8_t TMP = (CPU::InPortB(0x61)) & 0xFC;
		 
		 	CPU::OutPortB(0x61, TMP);
		}
	
		void Play(nat Frequency)
		{
			//Set the PIT to the desired frequency
			uint32_t Div = 1193180 / Frequency;
			CPU::OutPortB(0x43, 0xB6);
			CPU::OutPortB(0x42, (uint8_t) (Div) );
			CPU::OutPortB(0x42, (uint8_t) (Div >> 8));

			// And play the sound using the PC speaker
			uint8_t TMP = CPU::InPortB(0x61);

			if ( TMP != (TMP | 3) )
			{
				CPU::OutPortB(0x61, TMP | 3);
			}
		}

		void SysSetFrequency(CPU::InterruptRegisters* R)
		{
			unsigned frequency = R->ebx;
			if ( frequency == 0 ) { Mute(); } else { Play(frequency); }
		}
	}
}

