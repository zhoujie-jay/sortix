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
#include "syscall.h"
#include "process.h"

using namespace Maxsi;

namespace Sortix
{
	namespace VGA
	{
		addr_t SysCreateFrame();
		int SysChangeFrame(int fd);
		int SysDeleteFrame(int fd);

		uint16_t* const vga = (uint16_t* const) 0xB8000;
		const int width = 80;
		const int height = 80;

		DevVGAFrame* currentframe;

		void Init()
		{
			currentframe = NULL;

			Syscall::Register(SYSCALL_CREATE_FRAME, (void*) SysCreateFrame);
			Syscall::Register(SYSCALL_CHANGE_FRAME, (void*) SysChangeFrame);
			Syscall::Register(SYSCALL_DELETE_FRAME, (void*) SysDeleteFrame);
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

		addr_t SysCreateFrame()
		{
			addr_t page = Page::Get();
			if ( !page ) { return 0; }

			Process* process = CurrentProcess();
			addr_t mapto = process->AllocVirtualAddr(0x1000UL);
			UserFrame* userframe = (UserFrame*) mapto;

			// TODO: Check if mapto collides with any other memory section!

			if ( !Memory::MapUser(page, mapto) )
			{
				Page::Put(page); return 0;
			}

			Maxsi::Memory::Set(userframe, 0, sizeof(UserFrame));

			DevVGAFrame* frame = new DevVGAFrame();
			if ( frame == NULL )
			{
				Memory::UnmapUser(mapto);
				Page::Put(page); return 0;
			}

			int fd = process->descriptors.Allocate(frame);
			if ( fd < 0 )
			{
				delete frame;
				Memory::UnmapUser(mapto);
				Page::Put(page); return 0;
			}

			userframe->fd = fd;

			frame->process = process;
			frame->physical = page;
			frame->userframe = userframe;

			return mapto;
		}

		int SysChangeFrame(int fd)
		{
			Process* process = CurrentProcess();
			Device* device = process->descriptors.Get(fd);
			if ( !device ) { return -1; }

			if ( !device->IsType(Device::VGABUFFER) ) { return -2; }

			DevVGAFrame* frame = (DevVGAFrame*) device;

			ASSERT(frame->process == process);
			ASSERT(frame->physical != 0);
			ASSERT(frame->userframe != NULL);
			ASSERT(frame->onscreen == (frame == currentframe));

			// TODO: Check if userframe is actually user-space writable!

			// Check if we need to do anything.
			if ( frame == currentframe ) { return 0; }

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
					Memory::SwitchAddressSpace(currentframe->process->addrspace);
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
					Memory::SwitchAddressSpace(process->addrspace);
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

			return 0;
		}

		int SysDeleteFrame(int fd)
		{
			Process* process = CurrentProcess();
			Device* device = process->descriptors.Get(fd);
			process->descriptors.Free(fd);
			
			if ( device == NULL ) { return -1; }
			if ( !device->Close() )  { return -1; }
			
			return 0;
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
