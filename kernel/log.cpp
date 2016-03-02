/*
 * Copyright (c) 2011, 2012, 2013, 2014, 2015 Jonas 'Sortie' Termansen.
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
 * log.cpp
 * A system for logging various messages to the kernel log.
 */

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include <sortix/kernel/addralloc.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/log.h>
#include <sortix/kernel/pci.h>
#include <sortix/kernel/pci-mmio.h>
#include <sortix/kernel/textbuffer.h>

#include "lfbtextbuffer.h"
#include "multiboot.h"
#include "textterminal.h"
#include "vgatextbuffer.h"

namespace Sortix {
namespace Log {

uint8_t* fallback_framebuffer = NULL;
size_t fallback_framebuffer_bpp = 0;
size_t fallback_framebuffer_pitch = 0;
size_t fallback_framebuffer_width = 0;
size_t fallback_framebuffer_height = 0;

TextBufferHandle* device_textbufhandle = NULL;
size_t (*device_callback)(void*, const char*, size_t) = NULL;
size_t (*device_width)(void*) = NULL;
size_t (*device_height)(void*) = NULL;
void (*device_get_cursor)(void*, size_t*, size_t*) = NULL;
bool (*device_sync)(void*) = NULL;
void (*device_invalidate)(void*) = NULL;
void* device_pointer = NULL;
bool (*emergency_device_is_impaired)(void*) = NULL;
bool (*emergency_device_recoup)(void*) = NULL;
void (*emergency_device_reset)(void*) = NULL;
size_t (*emergency_device_callback)(void*, const char*, size_t) = NULL;
size_t (*emergency_device_width)(void*) = NULL;
void (*emergency_device_get_cursor)(void*, size_t*, size_t*) = NULL;
size_t (*emergency_device_height)(void*) = NULL;
bool (*emergency_device_sync)(void*) = NULL;
void* emergency_device_pointer = NULL;

static size_t PrintToTextTerminal(void* user, const char* str, size_t len)
{
	return ((TextTerminal*) user)->Print(str, len);
}

static size_t TextTermWidth(void* user)
{
	return ((TextTerminal*) user)->Width();
}

static size_t TextTermHeight(void* user)
{
	return ((TextTerminal*) user)->Height();
}

static void TextTermGetCursor(void* user, size_t* column, size_t* row)
{
	((TextTerminal*) user)->GetCursor(column, row);
}

static bool TextTermSync(void* user)
{
	return ((TextTerminal*) user)->Sync();
}

static void TextTermInvalidate(void* user)
{
	((TextTerminal*) user)->Invalidate();
}

static bool EmergencyTextTermIsImpaired(void* user)
{
	return ((TextTerminal*) user)->EmergencyIsImpaired();
}

static bool EmergencyTextTermRecoup(void* user)
{
	return ((TextTerminal*) user)->EmergencyRecoup();
}

static void EmergencyTextTermReset(void* user)
{
	((TextTerminal*) user)->EmergencyReset();
}

static
size_t EmergencyPrintToTextTerminal(void* user, const char* str, size_t len)
{
	return ((TextTerminal*) user)->EmergencyPrint(str, len);
}

static size_t EmergencyTextTermWidth(void* user)
{
	return ((TextTerminal*) user)->EmergencyWidth();
}

static size_t EmergencyTextTermHeight(void* user)
{
	return ((TextTerminal*) user)->EmergencyHeight();
}

static void EmergencyTextTermGetCursor(void* user, size_t* column, size_t* row)
{
	((TextTerminal*) user)->EmergencyGetCursor(column, row);
}

static bool EmergencyTextTermSync(void* user)
{
	return ((TextTerminal*) user)->EmergencySync();
}

void Init(multiboot_info_t* bootinfo)
{
	(void) bootinfo;

	const char* oom_msg = "Out of memory setting up kernel log.";

	// Create the backend text buffer.
	TextBuffer* textbuf;
	if ( (bootinfo->flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO) &&
	     bootinfo->framebuffer_type != MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT )
	{
		assert(bootinfo->framebuffer_type == MULTIBOOT_FRAMEBUFFER_TYPE_RGB);
		assert(bootinfo->framebuffer_bpp == 32);
		assert(0 < bootinfo->framebuffer_width);
		assert(0 < bootinfo->framebuffer_height);
		pcibar_t fakebar;
		fakebar.addr_raw = bootinfo->framebuffer_addr;
		fakebar.size_raw = (uint64_t) bootinfo->framebuffer_pitch * bootinfo->framebuffer_height;
		fakebar.addr_raw |= PCIBAR_TYPE_64BIT;
		addralloc_t fb_alloc;
		if ( !MapPCIBAR(&fb_alloc, fakebar, MAP_PCI_BAR_WRITE_COMBINE) )
			Panic("Framebuffer setup failure.");
		uint8_t* lfb = (uint8_t*) fb_alloc.from;
		uint32_t lfbformat = bootinfo->framebuffer_bpp;
		size_t scansize = bootinfo->framebuffer_pitch;
		textbuf = CreateLFBTextBuffer(lfb, lfbformat, bootinfo->framebuffer_width,
		                              bootinfo->framebuffer_height, scansize);
		if ( !textbuf )
			Panic(oom_msg);
		fallback_framebuffer = lfb;
		fallback_framebuffer_bpp = lfbformat;
		fallback_framebuffer_width = bootinfo->framebuffer_width;
		fallback_framebuffer_height = bootinfo->framebuffer_height;
		fallback_framebuffer_pitch = bootinfo->framebuffer_pitch;
	}
	else
	{
		uint16_t* const VGAFB = (uint16_t*) 0xB8000;
		const size_t VGA_WIDTH = 80;
		const size_t VGA_HEIGHT = 25;
		TextChar* vga_chars_buffer = new TextChar[VGA_WIDTH * VGA_HEIGHT];
		if ( !vga_chars_buffer )
			Panic(oom_msg);
		textbuf = new VGATextBuffer(VGAFB, vga_chars_buffer, VGA_WIDTH, VGA_HEIGHT);
		if ( !textbuf )
			Panic(oom_msg);
	}

	// Create the text buffer handle.
	device_textbufhandle = new TextBufferHandle(textbuf);
	if ( !device_textbufhandle )
		Panic(oom_msg);

	// Setup a text terminal instance.
	TextTerminal* textterm = new TextTerminal(device_textbufhandle);
	if ( !textterm )
		Panic(oom_msg);

	// Register the text terminal as the kernel log.
	Log::device_callback = PrintToTextTerminal;
	Log::device_width = TextTermWidth;
	Log::device_height = TextTermHeight;
	Log::device_get_cursor = TextTermGetCursor;
	Log::device_sync = TextTermSync;
	Log::device_invalidate = TextTermInvalidate;
	Log::device_pointer = textterm;

	// Register the emergency kernel log.
	Log::emergency_device_is_impaired = EmergencyTextTermIsImpaired;
	Log::emergency_device_recoup = EmergencyTextTermRecoup;
	Log::emergency_device_reset = EmergencyTextTermReset;
	Log::emergency_device_callback = EmergencyPrintToTextTerminal;
	Log::emergency_device_width = EmergencyTextTermWidth;
	Log::emergency_device_height = EmergencyTextTermHeight;
	Log::emergency_device_get_cursor = EmergencyTextTermGetCursor;
	Log::emergency_device_sync = EmergencyTextTermSync;
	Log::emergency_device_pointer = textterm;
}

void Center(const char* string)
{
	size_t log_width = Log::Width();
	while ( *string )
	{
		size_t string_width = strcspn(string, "\n");
		size_t leading = string_width <= log_width ?
		                 (log_width - string_width) / 2 : 0;
		for ( size_t i = 0; i < leading; i++ )
			Log::Print(" ");
		Log::PrintData(string, string_width);
		string += string_width;
		if ( *string == '\n' )
		{
			string++;
			Log::Print("\n");
		}
	}
}

} // namespace Log
} // namespace Sortix
