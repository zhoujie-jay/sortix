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

	vga.h
	A Video Graphics Array driver.

******************************************************************************/

#ifndef SORTIX_VGA_H
#define SORTIX_VGA_H

#include "device.h"

namespace Sortix
{
	class Process;

	namespace VGA
	{
		// TODO: Move these to a better place
		#define COLOR8_BLACK 0
		#define COLOR8_BLUE 1
		#define COLOR8_GREEN 2
		#define COLOR8_CYAN 3
		#define COLOR8_RED 4
		#define COLOR8_MAGENTA 5
		#define COLOR8_BROWN 6
		#define COLOR8_LIGHT_GREY 7
		#define COLOR8_DARK_GREY 8
		#define COLOR8_LIGHT_BLUE 9
		#define COLOR8_LIGHT_GREEN 10
		#define COLOR8_LIGHT_CYAN 11
		#define COLOR8_LIGHT_RED 12
		#define COLOR8_LIGHT_MAGENTA 13
		#define COLOR8_LIGHT_BROWN 14
		#define COLOR8_WHITE 15

		struct Frame
		{
			static const nat Mode = 0x3;
			static const size_t Width = 80;
			static const size_t Height = 25;

			uint16_t Data[80*25];
		};

		struct UserFrame : public Frame
		{
			int fd;
		};

		void Init();

		// System Calls.
		void SysCreateFrame(CPU::InterruptRegisters* R);
		void SysChangeFrame(CPU::InterruptRegisters* R);
		void SysDeleteFrame(CPU::InterruptRegisters* R);
	}

	class DevVGAFrame : public Device
	{
	public:
		virtual nat Flags();

	public:
		DevVGAFrame();
		~DevVGAFrame();

	public:
		Process* process;
		addr_t physical;
		VGA::UserFrame* userframe;
		bool onscreen;

	};
}

#endif

