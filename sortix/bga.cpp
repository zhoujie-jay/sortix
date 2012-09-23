/*******************************************************************************

	Copyright(C) Jonas 'Sortie' Termansen 2012.

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

	bga.cpp
	Driver for the Bochs VBE Extensions.

*******************************************************************************/

#include <sortix/kernel/platform.h>
#include <sortix/kernel/refcount.h>
#include <sortix/kernel/textbuffer.h>
#include <sortix/kernel/video.h>
#include <sortix/kernel/memorymanagement.h>
#include <sortix/kernel/pci.h>
#include <sortix/kernel/string.h>
#include <sortix/mman.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "x86-family/memorymanagement.h"
#include "lfbtextbuffer.h"
#include "cpu.h"
#include "bga.h"

namespace Sortix {
namespace BGA {

const bool TEST_RES_BY_TRYING = false;

const uint16_t VBE_DISPI_INDEX_ID = 0;
const uint16_t VBE_DISPI_INDEX_XRES = 1;
const uint16_t VBE_DISPI_INDEX_YRES = 2;
const uint16_t VBE_DISPI_INDEX_BPP = 3;
const uint16_t VBE_DISPI_INDEX_ENABLE = 4;
const uint16_t VBE_DISPI_INDEX_BANK = 5;
const uint16_t VBE_DISPI_INDEX_VIRT_WIDTH = 6;
const uint16_t VBE_DISPI_INDEX_VIRT_HEIGHT = 7;
const uint16_t VBE_DISPI_INDEX_X_OFFSET = 8;
const uint16_t VBE_DISPI_INDEX_Y_OFFSET = 9;

const uint16_t VBE_DISPI_IOPORT_INDEX = 0x01CE;
const uint16_t VBE_DISPI_IOPORT_DATA = 0x01CF;

const uint16_t VBE_DISPI_BPP_4 = 0x04;
const uint16_t VBE_DISPI_BPP_8 = 0x08;
const uint16_t VBE_DISPI_BPP_15 = 0x0F;
const uint16_t VBE_DISPI_BPP_16 = 0x10;
const uint16_t VBE_DISPI_BPP_24 = 0x18;
const uint16_t VBE_DISPI_BPP_32 = 0x20;

const uint16_t VBE_DISPI_DISABLED = 0x00;
const uint16_t VBE_DISPI_ENABLED = 0x01;
const uint16_t VBE_DISPI_GETCAPS = 0x02;
const uint16_t VBE_DISPI_8BIT_DAC = 0x20;
const uint16_t VBE_DISPI_LFB_ENABLED = 0x40;
const uint16_t VBE_DISPI_NOCLEARMEM = 0x80;

const uint16_t VBE_MIN_SUP_VERSION = 0xB0C0;
const uint16_t VBE_MIN_POS_VERSION = 0xB0C0;
const uint16_t VBE_MAX_POS_VERSION = 0xB0CF;

const size_t VBE_BANK_SIZE = 64UL * 1024UL;
volatile uint8_t* const VBE_VIDEO_MEM = (volatile uint8_t*) 0xA0000;

addr_t DetectBGAFramebuffer()
{
	uint32_t devaddr;
	pcifind_t pcifind;

	// Search for the bochs BGA device and compatible.
	memset(&pcifind, 255, sizeof(pcifind));
	pcifind.vendorid = 0x1234;
	pcifind.deviceid = 0x1111;
	if ( (devaddr = PCI::SearchForDevice(pcifind)) )
		return PCI::ParseDevBar0(devaddr);

	// Search for a generic VGA compatible device.
	memset(&pcifind, 255, sizeof(pcifind));
	pcifind.classid = 0x03;
	pcifind.subclassid = 0x00;
	pcifind.progif = 0x00;
	if ( (devaddr = PCI::SearchForDevice(pcifind)) )
		return PCI::ParseDevBar0(devaddr);

	return 0;
}

uint16_t version;
uint16_t maxbpp;
uint16_t maxxres;
uint16_t maxyres;

uint16_t curbpp;
uint16_t curxres;
uint16_t curyres;
uint16_t curbank;

addr_t bgaframebuffer;

void WriteRegister(uint16_t index, uint16_t value)
{
    CPU::OutPortW(VBE_DISPI_IOPORT_INDEX, index);
    CPU::OutPortW(VBE_DISPI_IOPORT_DATA, value);
}

uint16_t ReadRegister(uint16_t index)
{
    CPU::OutPortW(VBE_DISPI_IOPORT_INDEX, index);
    return CPU::InPortW(VBE_DISPI_IOPORT_DATA);
}

uint16_t GetCapability(uint16_t index)
{
	uint16_t wasenabled = ReadRegister(VBE_DISPI_INDEX_ENABLE);
	WriteRegister(VBE_DISPI_INDEX_ENABLE, wasenabled | VBE_DISPI_GETCAPS);
	uint16_t cap = ReadRegister(index);
	WriteRegister(VBE_DISPI_INDEX_ENABLE, wasenabled);
	return cap;
}

bool SetVideoMode(uint16_t width, uint16_t height, uint16_t depth, bool keep)
{
	bool uselinear = true;
	WriteRegister(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
	WriteRegister(VBE_DISPI_INDEX_XRES, width);
	WriteRegister(VBE_DISPI_INDEX_YRES, height);
	WriteRegister(VBE_DISPI_INDEX_BPP, depth);
	WriteRegister(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED |
        (uselinear ? VBE_DISPI_LFB_ENABLED : 0) |
        (keep ? VBE_DISPI_NOCLEARMEM : 0));
	curbpp = depth;
	curxres = width;
	curyres = height;
	return true;
}

bool IsStandardResolution(uint16_t width, uint16_t height, uint16_t depth)
{
	if ( depth != VBE_DISPI_BPP_32 ) { return false; }
	if ( width == 800 && height == 600 ) { return true; }
	if ( width == 1024 && height == 768 ) { return true; }
	if ( width == 1280 && height == 720 ) { return true; }
	if ( width == 1280 && height == 1024 ) { return true; }
	if ( width == 1600 && height == 900 ) { return true; }
	if ( width == 1920 && height == 1080 ) { return true; }
	return false;
}

// TODO: Need a better method of detecting available/desired resolutions.
bool SupportsResolution(uint16_t width, uint16_t height, uint16_t depth)
{
	if ( !width || !height || !depth ) { return false; }
	if ( maxxres < width || maxyres < height || maxbpp < depth ) return false;
	if ( width % 8U ) { return false; }
	uint16_t wasenabled = ReadRegister(VBE_DISPI_INDEX_ENABLE);
	if ( width == curxres && height == curyres && depth == curbpp) return true;
	if ( !TEST_RES_BY_TRYING ) { return true; }
	SetVideoMode(width, height, depth, true);
	uint16_t newxres = ReadRegister(VBE_DISPI_INDEX_XRES);
	uint16_t newyres = ReadRegister(VBE_DISPI_INDEX_XRES);
	uint16_t newbpp = ReadRegister(VBE_DISPI_INDEX_BPP);
	bool result = newxres != curxres || newyres != curyres || newbpp != curbpp;
	SetVideoMode(curxres, curyres, curbpp, true);
	WriteRegister(VBE_DISPI_INDEX_ENABLE, wasenabled);
	return result;
}

class BGADriver : public VideoDriver
{
public:
	BGADriver();
	virtual ~BGADriver();

public:
	virtual bool StartUp();
	virtual bool ShutDown();
	virtual char* GetCurrentMode() const;
	virtual bool SwitchMode(const char* mode);
	virtual bool Supports(const char* mode) const;
	virtual char** GetModes(size_t* nummodes) const;
	virtual off_t FrameSize() const;
	virtual ssize_t WriteAt(off_t off, const void* buf, size_t count);
	virtual ssize_t ReadAt(off_t off, void* buf, size_t count);
	virtual TextBuffer* CreateTextBuffer();

private:
	bool MapVideoMemory(size_t size);
	bool DetectModes() const;

private:
	mutable size_t nummodes;
	mutable char** modes;
	char* curmode;
	size_t lfbmapped;
	size_t framesize;

};

BGADriver::BGADriver()
{
	nummodes = 0;
	modes = NULL;
	curmode = NULL;
	lfbmapped = 0;
	framesize = 0;
}

BGADriver::~BGADriver()
{
	MapVideoMemory(0);
	for ( size_t i = 0; i < nummodes; i++ )
	{
		delete[] modes[i];
	}
	delete[] modes;
	delete[] curmode;
}

bool BGADriver::MapVideoMemory(size_t size)
{
	size = Page::AlignUp(size);
	addr_t phys = bgaframebuffer;
	addr_t mapat = Memory::GetVideoMemory();

	if ( size == lfbmapped )
		return true;

	if ( size < lfbmapped )
	{
		for ( size_t i = size; i < size; i += Page::Size() )
			Memory::Unmap(phys + i);
		Memory::Flush();
		lfbmapped = size;
		return true;
	}

	size_t maxsize = Memory::GetMaxVideoMemorySize();
	if ( maxsize < size )
	{
		Log::PrintF("Error: Insufficient virtual address space for BGA frame "
		            "of size 0x%zx bytes, only 0x%zx was available.\n", size,
		            maxsize);
		return false;
	}

	const addr_t mtype = Memory::PAT_WC;
	for ( size_t i = lfbmapped; i < size; i += Page::Size() )
		if ( !Memory::MapPAT(phys+i, mapat+i, PROT_KWRITE | PROT_KREAD, mtype) )
		{
			Log::PrintF("Error: Insufficient memory to map BGA framebuffer "
			            "onto kernel address space: needed 0x%zx bytes but "
			            "only 0x%zx was available at this point.\n", size, i);
			MapVideoMemory(lfbmapped); // Unmap what we added.
			return false;
		}
	Memory::Flush();
	lfbmapped = size;
	return true;
}

bool BGADriver::StartUp()
{
	if ( !modes && !DetectModes() ) { return false; }
	return true;
}

bool BGADriver::ShutDown()
{
	MapVideoMemory(0);
	if ( curmode )
	{
		delete[] curmode; curmode = NULL;
		errno = ENOSYS;
		return false; // TODO: Return to VGA Text Mode.
	}
	return true;
}

char* BGADriver::GetCurrentMode() const
{
	if ( !curmode ) { errno = EINVAL; return NULL; }
	return String::Clone(curmode);
}

bool BGADriver::SwitchMode(const char* mode)
{
	bool result = false;
	char* modeclone = String::Clone(mode);
	char* xstr = NULL;
	char* ystr = NULL;
	char* bppstr = NULL;
	if ( !ReadParamString(mode, "width", &xstr, "height", &ystr,
	                      "bpp", &bppstr, "STOP") ) { return false; }
	uint16_t xres = xstr ? atoi(xstr) : 0;
	uint16_t yres = ystr ? atoi(ystr) : 0;
	uint16_t bpp = bppstr ? atoi(bppstr) : 32;
	size_t newframesize = (size_t) xres * (size_t) yres * (size_t) bpp/8UL;

	// If the current resolution uses more memory than the new one, keep it
	// around in case setting the video mode failed.
	if ( MapVideoMemory(newframesize < lfbmapped ? lfbmapped : newframesize) &&
	     SetVideoMode(xres, yres, bpp, false) )
	{
		delete[] curmode;
		curmode = modeclone;
		modeclone = NULL;
		// We can now truncate the amount of memory to what we really need.
		MapVideoMemory(framesize = newframesize);
		result = true;
	}

	delete[] xstr;
	delete[] ystr;
	delete[] bppstr;
	delete[] modeclone;
	return result;
}

bool BGADriver::Supports(const char* mode) const
{
	char* xstr = NULL;
	char* ystr = NULL;
	char* bppstr = NULL;
	if ( !ReadParamString(mode, "width", &xstr, "height", &ystr,
	                      "bpp", &bppstr, NULL, NULL) ) { return false; }
	uint16_t xres = xstr ? atoi(xstr) : 0;
	uint16_t yres = ystr ? atoi(ystr) : 0;
	uint16_t bpp = bppstr ? atoi(bppstr) : 0;
	bool result = SupportsResolution(xres, yres, bpp);
	delete[] xstr;
	delete[] ystr;
	delete[] bppstr;
	return result;
}

char** BGADriver::GetModes(size_t* retnum) const
{
	if ( !modes && !DetectModes() ) { return NULL; }
	char** result = new char*[nummodes];
	if ( !result ) { return NULL; }
	for ( size_t i = 0; i < nummodes; i++ )
	{
		result[i] = String::Clone(modes[i]);
		if ( !result[i] )
		{
			for ( size_t n = 0; n < i; i++ ) { delete[] result[n]; }
			delete[] result;
			return NULL;
		}
	}
	*retnum = nummodes;
	return result;
}

off_t BGADriver::FrameSize() const
{
	return curxres * curyres * (curbpp / 8UL);
}

ssize_t BGADriver::WriteAt(off_t off, const void* buf, size_t count)
{
	uint8_t* frame = (uint8_t*) Memory::GetVideoMemory();
	if ( (off_t) framesize <= off )
		return 0;
	if ( framesize < off + count )
		count = framesize - off;
	memcpy(frame + off, buf, count);
	return count;
}

ssize_t BGADriver::ReadAt(off_t off, void* buf, size_t count)
{
	const uint8_t* frame = (const uint8_t*) Memory::GetVideoMemory();
	if ( (off_t) framesize <= off )
		return 0;
	if ( framesize < off + count )
		count = framesize - off;
	memcpy(buf, frame + off, count);
	return count;
}

bool BGADriver::DetectModes() const
{
	nummodes = 0;
	unsigned bpp = VBE_DISPI_BPP_32;
	for ( unsigned w = 0; w < maxxres; w += 4U )
	{
		for ( unsigned h = 0; h < maxyres; h += 4UL )
		{
			if ( !IsStandardResolution(w, h, bpp) ) { continue; }
			if ( !SupportsResolution(w, h, bpp) ) { continue; }
			nummodes++;
		}
	}
	modes = new char*[nummodes];
	if ( !modes ) { return false; }
	memset(modes, 0, sizeof(char*) * nummodes);
	size_t curmodeid = 0;
	for ( unsigned w = 0; w < maxxres; w += 4U )
	{
		for ( unsigned h = 0; h < maxyres; h += 4UL )
		{
			if ( !IsStandardResolution(w, h, bpp) ) { continue; }
			if ( !SupportsResolution(w, h, bpp) ) { continue; }
			char bppstr[64];
			char xresstr[64];
			char yresstr[64];
			snprintf(bppstr, 64, "%u", bpp);
			snprintf(xresstr, 64, "%u", w);
			snprintf(yresstr, 64, "%u", h);
			char* modestr = String::Combine(6, "width=", xresstr, ",height=",
			                                yresstr, ",bpp=", bppstr);
			if ( !modestr ) { return false; }
			modes[curmodeid++] = modestr;
		}
	}
	return true;
}

TextBuffer* BGADriver::CreateTextBuffer()
{
	uint8_t* lfb = (uint8_t*) Memory::GetVideoMemory();
	uint32_t lfbformat = curbpp;
	size_t scansize = curxres * curbpp / 8UL;
	return CreateLFBTextBuffer(lfb, lfbformat, curxres, curyres, scansize);
}

static uint16_t ProbeBGAVersion()
{
	// First see if the register is in the legal range.
	uint16_t ver = ReadRegister(VBE_DISPI_INDEX_ID);
	if ( ver < VBE_MIN_POS_VERSION )
		return 0;
	if ( ver > VBE_MAX_POS_VERSION )
		return 0;

	// The bootloader or BIOS may have set the current version to less than what
	// really is supported. If we a version number to the register, we can read
	// it back only if it is supported.

	// If the register accepts an invalid version number, don't trust it and we
	// may be in danger if an unrelated type of register is using this IO port.
	// Since that is unlikely, just assume we got a real BGA device.
	WriteRegister(VBE_DISPI_INDEX_ID, 0xFFFF);
	if ( ReadRegister(VBE_DISPI_INDEX_ID) == 0xFFFF )
	{
		WriteRegister(VBE_DISPI_INDEX_ID, ver);
		Log::PrintF("Warning: Found what appears to be BGA hardware, but it "
		            "behaves differently when attempting to scan what version "
		            "it conforms to. ");
		if ( ver < VBE_MIN_SUP_VERSION )
		{
			Log::PrintF("The hardware is by default set to an old unsupported "
			            "version, this driver will not use it.\n");
			return 0;
		}
		Log::PrintF("The driver will use this hardware (even though it may not "
		            "be BGA hardware) and bad things may happen.\n");
		return ver;
	}

	// Attempt to query all possible version ids.
	for ( uint16_t i = ver; i < VBE_MAX_POS_VERSION; i++ )
	{
		WriteRegister(VBE_DISPI_INDEX_ID, i);
		if ( ReadRegister(VBE_DISPI_INDEX_ID) == i )
			ver = i;
	}

	return ver;
}

void Init()
{
	if ( !(version = ProbeBGAVersion()) )
		return;

	curbpp = 0;
	curxres = 0;
	curyres = 0;
	curbank = 0xFFFF;

	maxbpp = GetCapability(VBE_DISPI_INDEX_BPP);
	maxxres = GetCapability(VBE_DISPI_INDEX_XRES);
	maxyres = GetCapability(VBE_DISPI_INDEX_YRES);

	if ( !(bgaframebuffer = DetectBGAFramebuffer()) )
	{
		Log::PrintF("BGA support detected but no PCI device could be found "
		            "determines the location of the framebuffer. Rather than "
		            "guessing it is at 0xE0000000, this driver shuts down to "
		            "avoid corrupting possible memory there.\n");
		return;
	}

	BGADriver* bgadriver = new BGADriver;
	if ( !bgadriver )
	{
		Log::PrintF("Unable to allocate BGA driver, but hardware present\n");
		return;
	}

	if ( !Video::RegisterDriver("bga", bgadriver) )
	{
		Log::PrintF("Unable to register BGA driver, but hardware present\n");
		delete bgadriver;
		return;
	}
}

} // namespace BGA
} // namespace Sortix
