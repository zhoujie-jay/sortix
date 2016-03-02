/*
 * Copyright (c) 2012, 2014, 2015 Jonas 'Sortie' Termansen.
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
 * video.cpp
 * Framework for Sortix video drivers.
 */

#include <stdarg.h>
#include <errno.h>
#include <string.h>

#include <sortix/kernel/copy.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/log.h>
#include <sortix/kernel/string.h>
#include <sortix/kernel/syscall.h>
#include <sortix/kernel/textbuffer.h>
#include <sortix/kernel/video.h>

namespace Sortix {
namespace Video {

const uint64_t ONE_AND_ONLY_DEVICE = 0;
const uint64_t ONE_AND_ONLY_CONNECTOR = 0;

kthread_mutex_t video_lock = KTHREAD_MUTEX_INITIALIZER;

struct DeviceEntry
{
	char* name;
	VideoDevice* device;
};

size_t num_devices = 0;
size_t devices_length = 0;
DeviceEntry* devices = NULL;

bool RegisterDevice(const char* name, VideoDevice* device)
{
	ScopedLock lock(&video_lock);
	if ( num_devices == devices_length )
	{
		size_t newdevices_length = devices_length ? 2 * devices_length : 8UL;
		DeviceEntry* newdevices = new DeviceEntry[newdevices_length];
		if ( !newdevices )
			return false;
		if ( devices )
		{
			memcpy(newdevices, devices, sizeof(*devices) * num_devices);
			delete[] devices;
		}
		devices = newdevices;
		devices_length = devices_length;
	}

	char* drivername = String::Clone(name);
	if ( !drivername )
		return false;

	size_t index = num_devices++;
	devices[index].name = drivername;
	devices[index].device = device;
	return true;
}

__attribute__((unused))
static bool TransmitString(struct dispmsg_string* dest, const char* str)
{
	size_t size = strlen(str) + 1;
	size_t dest_size = dest->byte_size;
	dest->byte_size = size;
	if ( dest_size < size )
		return errno = ERANGE, false;
	return CopyToUser(dest->str, str, size);
}

__attribute__((unused))
static char* ReceiveString(struct dispmsg_string* src)
{
	if ( !src->byte_size )
		return errno = EINVAL, (char*) NULL;
	char* ret = new char[src->byte_size];
	if ( !ret )
		return NULL;
	if ( !CopyFromUser(ret, src->str, src->byte_size) )
		return NULL;
	if ( ret[src->byte_size-1] != '\0' )
	{
		delete[] ret;
		return errno = EINVAL, (char*) NULL;
	}
	return ret;
}

static int EnumerateDevices(void* ptr, size_t size)
{
	struct dispmsg_enumerate_devices msg;
	if ( size != sizeof(msg) )
		return errno = EINVAL, -1;
	if ( !CopyFromUser(&msg, ptr, sizeof(msg)) )
		return -1;

	ScopedLock lock(&video_lock);

	size_t requested_num_devices = msg.devices_length;
	msg.devices_length = num_devices;

	if ( !CopyToUser(ptr, &msg, sizeof(msg)) )
		return -1;

	if ( requested_num_devices < num_devices )
		return errno = ERANGE, -1;

	for ( uint64_t i = 0; i < num_devices; i++ )
		if ( !CopyToUser(&msg.devices[i], &i, sizeof(i)) )
			return -1;

	return 0;
}

static int GetDriverCount(void* ptr, size_t size)
{
	struct dispmsg_get_driver_count msg;
	if ( size != sizeof(msg) )
		return errno = EINVAL, -1;
	if ( !CopyFromUser(&msg, ptr, sizeof(msg)) )
		return -1;

	ScopedLock lock(&video_lock);

	if ( num_devices <= msg.device )
		return errno = ENODEV, -1;

	msg.driver_count = 1;

	if ( !CopyToUser(ptr, &msg, sizeof(msg)) )
		return -1;

	return 0;
}

static int GetDriverName(void* ptr, size_t size)
{
	struct dispmsg_get_driver_name msg;
	if ( size != sizeof(msg) )
		return errno = EINVAL, -1;
	if ( !CopyFromUser(&msg, ptr, sizeof(msg)) )
		return -1;

	ScopedLock lock(&video_lock);

	if ( num_devices <= msg.device )
		return errno = ENODEV, -1;

	DeviceEntry* device_entry = &devices[msg.device];
	if ( !TransmitString(&msg.name, device_entry->name) )
		return -1;

	if ( !CopyToUser(ptr, &msg, sizeof(msg)) )
		return -1;

	return 0;
}

static int GetDriver(void* ptr, size_t size)
{
	struct dispmsg_get_driver msg;
	if ( size != sizeof(msg) )
		return errno = EINVAL, -1;
	if ( !CopyFromUser(&msg, ptr, sizeof(msg)) )
		return -1;

	ScopedLock lock(&video_lock);

	if ( num_devices <= msg.device )
		return errno = ENODEV, -1;

	msg.driver_index = 0;

	if ( !CopyToUser(ptr, &msg, sizeof(msg)) )
		return -1;

	return 0;
}

static int SetDriver(void* ptr, size_t size)
{
	struct dispmsg_set_driver msg;
	if ( size != sizeof(msg) )
		return errno = EINVAL, -1;
	if ( !CopyFromUser(&msg, ptr, sizeof(msg)) )
		return -1;

	ScopedLock lock(&video_lock);

	if ( num_devices <= msg.device )
		return errno = ENODEV, -1;

	if ( msg.driver_index != 0 )
		return errno = EINVAL, -1;

	if ( !CopyToUser(ptr, &msg, sizeof(msg)) )
		return -1;

	return 0;
}

static struct dispmsg_crtc_mode GetLogFallbackMode()
{
	struct dispmsg_crtc_mode mode;
	memset(&mode, 0, sizeof(mode));
	mode.control = DISPMSG_CONTROL_VALID;
	mode.fb_format = Log::fallback_framebuffer_bpp;
	mode.view_xres = (uint32_t) Log::fallback_framebuffer_width;
	mode.view_yres = (uint32_t) Log::fallback_framebuffer_height;
	mode.fb_location = 0;
	mode.pitch = Log::fallback_framebuffer_width *
	             Log::fallback_framebuffer_bpp / 8;
	return mode;
}

static int SetCrtcMode(void* ptr, size_t size)
{
	struct dispmsg_set_crtc_mode msg;
	if ( size != sizeof(msg) )
		return errno = EINVAL, -1;
	if ( !CopyFromUser(&msg, ptr, sizeof(msg)) )
		return -1;

	ScopedLock lock(&video_lock);

	if ( msg.device < num_devices )
	{
		DeviceEntry* device_entry = &devices[msg.device];
		VideoDevice* device = device_entry->device;
		if ( !device->SwitchMode(msg.connector, msg.mode) )
			return -1;
		// TODO: This could potentially fail.
		if ( msg.device == ONE_AND_ONLY_DEVICE &&
			 msg.connector == ONE_AND_ONLY_CONNECTOR )
		{
			Log::fallback_framebuffer = NULL;
			Log::device_textbufhandle->Replace(device->CreateTextBuffer(msg.connector));
		}
	}
	else if ( Log::fallback_framebuffer &&
	          msg.device == ONE_AND_ONLY_DEVICE &&
	          msg.connector == ONE_AND_ONLY_CONNECTOR )
	{
		struct dispmsg_crtc_mode fallback_mode = GetLogFallbackMode();
		if ( memcmp(&msg.mode, &fallback_mode, sizeof(msg.mode)) != 0 )
			return errno = EINVAL, -1;
	}
	else
	{
		return errno = ENODEV, -1;
	}

	// No need to respond.

	return 0;
}

static int GetCrtcMode(void* ptr, size_t size)
{
	struct dispmsg_get_crtc_mode msg;
	if ( size != sizeof(msg) )
		return errno = EINVAL, -1;
	if ( !CopyFromUser(&msg, ptr, sizeof(msg)) )
		return -1;

	ScopedLock lock(&video_lock);

	struct dispmsg_crtc_mode mode;
	if ( Log::fallback_framebuffer &&
	     msg.device == ONE_AND_ONLY_DEVICE &&
	     msg.connector == ONE_AND_ONLY_CONNECTOR )
	{
		mode = GetLogFallbackMode();
	}
	else if ( msg.device < num_devices )
	{
		DeviceEntry* device_entry = &devices[msg.device];
		VideoDevice* device = device_entry->device;
		// TODO: There is no real way to detect failure here.
		errno = 0;
		mode = device->GetCurrentMode(msg.connector);
		if ( !(mode.control & DISPMSG_CONTROL_VALID) && errno != 0 )
			return -1;
	}
	else
	{
		return errno = ENODEV, -1;
	}

	msg.mode = mode;

	if ( !CopyToUser(ptr, &msg, sizeof(msg)) )
		return -1;

	return 0;
}

static int GetCrtcModes(void* ptr, size_t size)
{
	struct dispmsg_get_crtc_modes msg;
	if ( size != sizeof(msg) )
		return errno = EINVAL, -1;
	if ( !CopyFromUser(&msg, ptr, sizeof(msg)) )
		return -1;

	ScopedLock lock(&video_lock);

	size_t nummodes = 0;
	struct dispmsg_crtc_mode* modes = NULL;
	if ( msg.device < num_devices )
	{
		DeviceEntry* device_entry = &devices[msg.device];
		VideoDevice* device = device_entry->device;
		modes = device->GetModes(msg.connector, &nummodes);
		if ( !modes )
			return -1;
	}
	else if ( Log::fallback_framebuffer &&
	          msg.device == ONE_AND_ONLY_DEVICE &&
	          msg.connector == ONE_AND_ONLY_CONNECTOR )
	{
		if ( !(modes = new struct dispmsg_crtc_mode[1]) )
			return -1;
		modes[nummodes++] = GetLogFallbackMode();
	}
	else
	{
		return errno = ENODEV, -1;
	}

	size_t requested_modes = msg.modes_length;
	msg.modes_length = nummodes;

	if ( !CopyToUser(ptr, &msg, sizeof(msg)) )
		return -1;

	if ( requested_modes < nummodes )
	{
		delete[] modes;
		return errno = ERANGE, -1;
	}

	for ( size_t i = 0; i < nummodes; i++ )
	{
		if ( !CopyToUser(&msg.modes[i], &modes[i], sizeof(modes[i])) )
		{
			delete[] modes;
			return -1;
		}
	}

	delete[] modes;

	return 0;
}

static int GetMemorySize(void* ptr, size_t size)
{
	struct dispmsg_get_memory_size msg;
	if ( size != sizeof(msg) )
		return errno = EINVAL, -1;
	if ( !CopyFromUser(&msg, ptr, sizeof(msg)) )
		return -1;

	ScopedLock lock(&video_lock);

	if ( Log::fallback_framebuffer &&
	     msg.device == ONE_AND_ONLY_DEVICE )
	{
		msg.memory_size = Log::fallback_framebuffer_width *
		                  Log::fallback_framebuffer_height *
		                  Log::fallback_framebuffer_bpp / 8;
	}
	else if ( msg.device < num_devices )
	{
		DeviceEntry* device_entry = &devices[msg.device];
		VideoDevice* device = device_entry->device;
		msg.memory_size = device->FrameSize();
	}
	else
	{
		return errno = ENODEV, -1;
	}

	DeviceEntry* device_entry = &devices[msg.device];
	VideoDevice* device = device_entry->device;

	msg.memory_size = device->FrameSize();

	if ( !CopyToUser(ptr, &msg, sizeof(msg)) )
		return -1;

	return 0;
}

static int WriteMemory(void* ptr, size_t size)
{
	struct dispmsg_write_memory msg;
	if ( size != sizeof(msg) )
		return errno = EINVAL, -1;
	if ( !CopyFromUser(&msg, ptr, sizeof(msg)) )
		return -1;

	ScopedLock lock(&video_lock);

	if ( Log::fallback_framebuffer &&
	     msg.device == ONE_AND_ONLY_DEVICE )
	{
		size_t ideal_pitch = Log::fallback_framebuffer_width *
		                     Log::fallback_framebuffer_bpp / 8;
		if ( ideal_pitch == Log::fallback_framebuffer_pitch )
		{
			size_t framesize = Log::fallback_framebuffer_height *
		                       Log::fallback_framebuffer_pitch;
			if ( framesize < msg.offset )
				return 0;
			size_t count = msg.size;
			size_t left = framesize - msg.offset;
			if ( left < count )
				count = left;
			const uint8_t* src = msg.src;
			uint8_t* dst = Log::fallback_framebuffer + msg.offset;
			if ( !CopyFromUser(dst, src, count) )
				return -1;
		}
		else
		{
			const uint8_t* src = msg.src;
			size_t src_size = msg.size;
			uint64_t y = msg.offset / ideal_pitch;
			uint64_t x = msg.offset % ideal_pitch;
			while ( src_size && y < Log::fallback_framebuffer_height )
			{
				size_t count = src_size;
				if ( ideal_pitch -x < count )
					count = ideal_pitch - x;
				uint8_t* dst = Log::fallback_framebuffer +
				               Log::fallback_framebuffer_pitch * y + x;
				if ( !CopyFromUser(dst, src, count) )
					return -1;
				src += count;
				src_size -= count;
				x = 0;
				y++;
			}
		}
	}
	else if ( msg.device < num_devices )
	{
		DeviceEntry* device_entry = &devices[msg.device];
		VideoDevice* device = device_entry->device;
		ioctx_t ctx; SetupUserIOCtx(&ctx);
		if ( device->WriteAt(&ctx, msg.offset, msg.src, msg.size) < 0 )
			return -1;
	}
	else
	{
		return errno = ENODEV, -1;
	}

	Log::Invalidate();

	return 0;
}

static int ReadMemory(void* ptr, size_t size)
{
	struct dispmsg_read_memory msg;
	if ( size != sizeof(msg) )
		return errno = EINVAL, -1;
	if ( !CopyFromUser(&msg, ptr, sizeof(msg)) )
		return -1;

	ScopedLock lock(&video_lock);

	if ( Log::fallback_framebuffer &&
	     msg.device == ONE_AND_ONLY_DEVICE )
	{
		size_t ideal_pitch = Log::fallback_framebuffer_width *
		                     Log::fallback_framebuffer_bpp / 8;
		if ( ideal_pitch == Log::fallback_framebuffer_pitch )
		{
			size_t framesize = Log::fallback_framebuffer_height *
		                       Log::fallback_framebuffer_pitch;
			if ( framesize < msg.offset )
				return 0;
			size_t count = msg.size;
			size_t left = framesize - msg.offset;
			if ( left < count )
				count = left;
			const uint8_t* src = Log::fallback_framebuffer + msg.offset;
			uint8_t* dst = msg.dst;
			if ( !CopyToUser(dst, src, count) )
				return -1;
		}
		else
		{
			uint8_t* dst = msg.dst;
			size_t dst_size = msg.size;
			uint64_t y = msg.offset / ideal_pitch;
			uint64_t x = msg.offset % ideal_pitch;
			while ( dst_size && y < Log::fallback_framebuffer_height )
			{
				size_t count = dst_size;
				if ( ideal_pitch -x < count )
					count = ideal_pitch - x;
				const uint8_t* src = Log::fallback_framebuffer +
				                     Log::fallback_framebuffer_pitch * y + x;
				if ( !CopyToUser(dst, src, count) )
					return -1;
				dst += count;
				dst_size -= count;
				x = 0;
				y++;
			}
		}
	}
	else if ( msg.device < num_devices )
	{
		DeviceEntry* device_entry = &devices[msg.device];
		VideoDevice* device = device_entry->device;
		ioctx_t ctx; SetupUserIOCtx(&ctx);
		if ( device->ReadAt(&ctx, msg.offset, msg.dst, msg.size) < 0 )
			return -1;
	}
	else
	{
		return errno = ENODEV, -1;
	}

	return 0;
}

} // namespace Video
} // namespace Sortix

namespace Sortix {

int sys_dispmsg_issue(void* ptr, size_t size)
{
	using namespace Video;

	struct dispmsg_header hdr;
	if ( size < sizeof(hdr) )
		return errno = EINVAL, -1;
	if ( !CopyFromUser(&hdr, ptr, sizeof(hdr)) )
		return -1;
	switch ( hdr.msgid )
	{
	case DISPMSG_ENUMERATE_DEVICES: return EnumerateDevices(ptr, size);
	case DISPMSG_GET_DRIVER_COUNT: return GetDriverCount(ptr, size);
	case DISPMSG_GET_DRIVER_NAME: return GetDriverName(ptr, size);
	case DISPMSG_GET_DRIVER: return GetDriver(ptr, size);
	case DISPMSG_SET_DRIVER: return SetDriver(ptr, size);
	case DISPMSG_SET_CRTC_MODE: return SetCrtcMode(ptr, size);
	case DISPMSG_GET_CRTC_MODE: return GetCrtcMode(ptr, size);
	case DISPMSG_GET_CRTC_MODES: return GetCrtcModes(ptr, size);
	case DISPMSG_GET_MEMORY_SIZE: return GetMemorySize(ptr, size);
	case DISPMSG_WRITE_MEMORY: return WriteMemory(ptr, size);
	case DISPMSG_READ_MEMORY: return ReadMemory(ptr, size);
	default:
		return errno = ENOSYS, -1;
	}
}

} // namespace Sortix
