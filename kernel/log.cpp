/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2014, 2015.

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

    log.cpp
    A system for logging various messages to the kernel log.

*******************************************************************************/

#include <stddef.h>
#include <string.h>

#include <sortix/kernel/kernel.h>
#include <sortix/kernel/log.h>
#include <sortix/kernel/textbuffer.h>

#include "textterminal.h"
#include "vgatextbuffer.h"

namespace Sortix {
namespace Log {

TextBufferHandle* device_textbufhandle = NULL;
size_t (*device_callback)(void*, const char*, size_t) = NULL;
size_t (*device_width)(void*) = NULL;
size_t (*device_height)(void*) = NULL;
void (*device_get_cursor)(void*, size_t*, size_t*) = NULL;
bool (*device_sync)(void*) = NULL;
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
	if ( true )
	{
		uint16_t* const VGAFB = (uint16_t*) 0xB8000;
		const size_t VGA_WIDTH = 80;
		const size_t VGA_HEIGHT = 25;
		uint16_t* vga_attr_buffer = new uint16_t[VGA_WIDTH * VGA_HEIGHT];
		TextChar* vga_chars_buffer = new TextChar[VGA_WIDTH * VGA_HEIGHT];
		if ( !vga_attr_buffer || !vga_chars_buffer )
			Panic(oom_msg);
		textbuf = new VGATextBuffer(VGAFB, vga_chars_buffer, vga_attr_buffer,
		                            VGA_WIDTH, VGA_HEIGHT);
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

} // namespace Log
} // namespace Sortix
