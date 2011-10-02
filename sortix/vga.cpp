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

#include "platform.h"
#include <libmaxsi/memory.h>
#include "vga.h"
#include "memorymanagement.h"
#include "scheduler.h"

using namespace Maxsi;

namespace Sortix
{
	namespace VGA
	{
		uint16_t* const vga = (uint16_t* const) 0xB8000;
		const int width = 80;
		const int height = 80;

		DevVGAFrame* currentframe;

		void Init()
		{
			currentframe = NULL;
		}

		// Changes the position of the hardware cursor.
		void SetCursor(nat x, nat y)
		{
			nat value = x + y * width;

			// This sends a command to indicies 14 and 15 in the
			// CRT Control Register of the VGA controller. These
			// are the high and low bytes of the index that show
			// where the hardware cursor is to be 'blinking'.
			CPU::OutPortB(0x3D4, 14);
			CPU::OutPortB(0x3D5, (value >> 8) & 0xFF);
			CPU::OutPortB(0x3D4, 15);
			CPU::OutPortB(0x3D5, (value >> 0) & 0xFF);
		}

		void SysCreateFrame(CPU::InterruptRegisters* R)
		{
#ifdef PLATFORM_X86
			addr_t page = Page::Get();
			if ( page == NULL ) { R->eax = 0; return; }

			Process* process = CurrentProcess();
			addr_t mapto = Page::AlignUp(process->_endcodesection);
			UserFrame* userframe = (UserFrame*) mapto;

			// TODO: Check if mapto collides with any other memory section!

			if ( !Memory::MapUser(page, mapto) )
			{
				Page::Put(page); R->eax = 0; return;
			}

			Maxsi::Memory::Set(userframe, 0, sizeof(UserFrame));

			DevVGAFrame* frame = new DevVGAFrame();
			if ( frame == NULL )
			{
				Memory::UnmapUser(mapto);
				Page::Put(page); R->eax = 0; return;
			}

			int fd = process->descriptors.Allocate(frame);
			if ( fd < 0 )
			{
				delete frame;
				Memory::UnmapUser(mapto);
				Page::Put(page); R->eax = 0; return;
			}

			userframe->fd = fd;

			frame->process = process;
			frame->physical = page;
			frame->userframe = userframe;

			process->_endcodesection = mapto + 0x1000UL;

			R->eax = mapto;
#endif
		}

		void SysChangeFrame(CPU::InterruptRegisters* R)
		{
#ifdef PLATFORM_X86
			int fd = (int) R->ebx;

			Process* process = CurrentProcess();
			Device* device = process->descriptors.Get(fd);
			if ( device == NULL ) { R->eax = -1; return; }

			if ( !device->IsType(Device::VGABUFFER) ) { R->eax = -2; return; }

			DevVGAFrame* frame = (DevVGAFrame*) device;

			ASSERT(frame->process == process);
			ASSERT(frame->physical != 0);
			ASSERT(frame->userframe != NULL);
			ASSERT(frame->onscreen == (frame == currentframe));

			// TODO: Check if userframe is actually user-space writable!

			//Log::PrintF("changeframe: fd = %u, frame = 0x%p, currentframe = 0x%p, userframe = 0x%p\n", fd, frame, currentframe, frame->userframe); while(true);

			// Check if we need to do anything.
			if ( frame == currentframe ) { R->eax = 0; return; }

			// If there is already a currently used frame? If so, swap it from
			// the VGA memory and back to the RAM. This should be done
			// transparently such that the program doesn't feel the difference. 
			if ( currentframe != NULL )
			{
				ASSERT(currentframe->physical != frame->physical);
				ASSERT(currentframe->userframe != frame->userframe);
				ASSERT(currentframe->onscreen == true);

				if ( currentframe->process != process )
				{
					Memory::SwitchAddressSpace(currentframe->process->GetAddressSpace());
				}

				// Remap the pages in the owning process.
				// TODO: Check if userframe is actually user-space writable!
				Memory::UnmapUser((addr_t) currentframe->userframe);
				Memory::MapUser(currentframe->physical, (addr_t) currentframe->userframe);
				Memory::InvalidatePage((addr_t) frame->userframe);

				// Restore the contents of this frame to the VGA framebuffer.
				Maxsi::Memory::Copy(currentframe->userframe, vga, sizeof(UserFrame));

				if ( currentframe->process != process )
				{
					Memory::SwitchAddressSpace(process->GetAddressSpace());
				}

				currentframe->onscreen = false;
			}

			// Now move the contents of this frame to the VGA framebuffer.
			Maxsi::Memory::Copy(vga, frame->userframe, sizeof(UserFrame));

			// Remap the pages such that the current process now uses the vga.
			Memory::UnmapUser((addr_t) frame->userframe);
			Memory::MapUser((addr_t) vga, (addr_t) frame->userframe);
			Memory::InvalidatePage((addr_t) frame->userframe);

			frame->onscreen = true;
			currentframe = frame;
			SetCursor(width, height-1);
#endif
		}

		void SysDeleteFrame(CPU::InterruptRegisters* R)
		{
#ifdef PLATFORM_X86
			int fd = (int) R->ebx;

			Process* process = CurrentProcess();
			Device* device = process->descriptors.Get(fd);
			process->descriptors.Free(fd);
			
			if ( device == NULL ) { R->eax = -1; return; }
			if ( !device->Close() )  { R->eax = -1; return; }
			R->eax = 0;
#endif
		}
	}

	DevVGAFrame::DevVGAFrame()
	{
		process = NULL;
		userframe = NULL;
		physical = 0;
		onscreen = false;
	}

	DevVGAFrame::~DevVGAFrame()
	{
		if ( process != NULL ) { ASSERT(CurrentProcess() == process); }
		if ( userframe != NULL ) { Memory::UnmapUser((addr_t) userframe); Memory::InvalidatePage((addr_t) userframe); }
		if ( physical != 0 ) { Page::Put(physical); }
	}

	nat DevVGAFrame::Flags() { return Device::VGABUFFER; }
}
