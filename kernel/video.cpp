/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012, 2014.

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

    video.cpp
    Framework for Sortix video drivers.

*******************************************************************************/

#include <stdarg.h>
#include <errno.h>
#include <string.h>

#include <sortix/kernel/copy.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/kthread.h>
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

Ref<TextBufferHandle> textbufhandle;

bool RegisterDevice(const char* name, VideoDevice* device)
{
	ScopedLock lock(&video_lock);
	if ( num_devices == devices_length )
	{
		size_t newdevices_length = devices_length ? 2 * devices_length : 8UL;
		DeviceEntry* newdevices = new DeviceEntry[newdevices_length];
		if ( !newdevices )
			return false;
		memcpy(newdevices, devices, sizeof(*devices) * num_devices);
		delete[] devices; devices = newdevices;
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

static int SetCrtcMode(void* ptr, size_t size)
{
	struct dispmsg_set_crtc_mode msg;
	if ( size != sizeof(msg) )
		return errno = EINVAL, -1;
	if ( !CopyFromUser(&msg, ptr, sizeof(msg)) )
		return -1;

	ScopedLock lock(&video_lock);

	if ( num_devices <= msg.device )
		return errno = ENODEV, -1;

	DeviceEntry* device_entry = &devices[msg.device];
	VideoDevice* device = device_entry->device;
	if ( !device->SwitchMode(msg.connector, msg.mode) )
		return -1;

	// TODO: This could potentially fail.
	if ( msg.device == ONE_AND_ONLY_DEVICE &&
	     msg.connector == ONE_AND_ONLY_CONNECTOR )
		textbufhandle->Replace(device->CreateTextBuffer(msg.connector));

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

	if ( num_devices <= msg.device )
		return errno = ENODEV, -1;

	DeviceEntry* device_entry = &devices[msg.device];
	VideoDevice* device = device_entry->device;

	// TODO: There is no real way to detect failure here.
	errno = 0;
	struct dispmsg_crtc_mode mode = device->GetCurrentMode(msg.connector);
	if ( !(mode.control & DISPMSG_CONTROL_VALID) && errno != 0 )
		return -1;

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

	if ( num_devices <= msg.device )
		return errno = ENODEV, -1;

	DeviceEntry* device_entry = &devices[msg.device];
	VideoDevice* device = device_entry->device;

	size_t nummodes;
	struct dispmsg_crtc_mode* modes = device->GetModes(msg.connector, &nummodes);
	if ( !modes )
		return -1;

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

	if ( num_devices <= msg.device )
		return errno = ENODEV, -1;

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

	if ( num_devices <= msg.device )
		return errno = ENODEV, -1;

	DeviceEntry* device_entry = &devices[msg.device];
	VideoDevice* device = device_entry->device;

	ioctx_t ctx; SetupUserIOCtx(&ctx);
	if ( device->WriteAt(&ctx, msg.offset, msg.src, msg.size) < 0 )
		return -1;

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

	if ( num_devices <= msg.device )
		return errno = ENODEV, -1;

	DeviceEntry* device_entry = &devices[msg.device];
	VideoDevice* device = device_entry->device;

	ioctx_t ctx; SetupUserIOCtx(&ctx);
	if ( device->ReadAt(&ctx, msg.offset, msg.dst, msg.size) < 0 )
		return -1;

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

namespace Sortix {
namespace Video {

void Init(Ref<TextBufferHandle> thetextbufhandle)
{
	textbufhandle = thetextbufhandle;
}

} // namespace Video
} // namespace Sortix
