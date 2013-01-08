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

    dispmsg.cpp
    User-space message-based interface for video framework access.

*******************************************************************************/

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <sortix/display.h>
#include <sortix/syscallnum.h>

#include <sortix/kernel/platform.h>
#include <sortix/kernel/video.h>
#include <sortix/kernel/string.h>
#include <sortix/kernel/syscall.h>

#include "dispmsg.h"

namespace Sortix {
namespace DisplayMessage {

const uint64_t ONE_AND_ONLY_DEVICE = 0;
const uint64_t ONE_AND_ONLY_CONNECTOR = 0;

__attribute__((unused))
static char* StringOfCrtcMode(struct dispmsg_crtc_mode mode)
{
	char bppstr[sizeof(mode.fb_format) * 3];
	char xresstr[sizeof(mode.view_xres) * 3];
	char yresstr[sizeof(mode.view_yres) * 3];
	char magicstr[sizeof(mode.magic) * 3];
	snprintf(bppstr, sizeof(bppstr), "%ju", (uintmax_t) mode.fb_format);
	snprintf(xresstr, sizeof(xresstr), "%ju", (uintmax_t) mode.view_xres);
	snprintf(yresstr, sizeof(yresstr), "%ju", (uintmax_t) mode.view_yres);
	snprintf(magicstr, sizeof(magicstr), "%ju", (uintmax_t) mode.magic);
	char* drivername = Video::GetDriverName(mode.driver_index);
	if ( !drivername )
		return NULL;
	char* ret = String::Combine(10,
	                            "driver=", drivername, ","
	                            "bpp=", bppstr, ","
	                            "width=", xresstr, ","
	                            "height=", yresstr, ","
	                            "modeid=", magicstr);
	delete[] drivername;
	return ret;
}

__attribute__((unused))
struct dispmsg_crtc_mode CrtcModeOfString(const char* string, uint64_t driverid)
{
	char* xstr = NULL;
	char* ystr = NULL;
	char* bppstr = NULL;
	char* modeidstr = NULL;
	char* unsupportedstr = NULL;
	struct dispmsg_crtc_mode ret;
	ret.control = 0;
	if ( !strcmp(string, "driver=none") )
		return ret;
	if ( !ReadParamString(string,
	                      "width", &xstr,
	                      "height", &ystr,
	                      "bpp", &bppstr,
	                      "modeid", &modeidstr,
	                      "unsupported", &unsupportedstr,
	                      NULL, NULL) )
		return ret;
	ret.driver_index = driverid;
	ret.view_xres = xstr ? atol(xstr) : 0;
	delete[] xstr;
	ret.view_yres = ystr ? atol(ystr) : 0;
	delete[] ystr;
	ret.fb_format = bppstr ? atol(bppstr) : 0;
	delete[] bppstr;
	ret.magic = modeidstr ? atoll(modeidstr) : 0;
	delete[] modeidstr;
	if ( !unsupportedstr )
		ret.control = 1;
	delete[] unsupportedstr;
	return ret;
}

__attribute__((unused))
static bool TransmitString(struct dispmsg_string* dest, const char* str)
{
	size_t size = strlen(str) + 1;
	size_t dest_size = dest->byte_size;
	dest->byte_size = size;
	if ( dest_size < size )
		return errno = ERANGE, false;
	strcpy(dest->str, str);
	return true;
}

__attribute__((unused))
static char* ReceiveString(struct dispmsg_string* src)
{
	if ( !src->byte_size )
bad_input:
		return errno = EINVAL, (char*) NULL;
	char* ret = new char[src->byte_size];
	if ( !ret )
		return NULL;
	memcpy(ret, src->str, src->byte_size);
	if ( ret[src->byte_size-1] != '\0' ) { delete[] ret; goto bad_input; }
	return ret;
}

static int EnumerateDevices(void* ptr, size_t size)
{
	if ( size != sizeof(struct dispmsg_enumerate_devices) )
		return errno = EINVAL, -1;
	struct dispmsg_enumerate_devices* msg =
		(struct dispmsg_enumerate_devices*) ptr;
	// TODO: HACK: Only one device is currently supported by our backend.
	size_t num_devices = 1;
	if ( msg->devices_length < num_devices )
	{
		msg->devices_length = num_devices;
		return errno = ERANGE, -1;
	}
	msg->devices[0] = ONE_AND_ONLY_DEVICE;
	return 0;
}

static int GetDriverCount(void* ptr, size_t size)
{
	if ( size != sizeof(struct dispmsg_get_driver_count) )
		return errno = EINVAL, -1;
	struct dispmsg_get_driver_count* msg =
		(struct dispmsg_get_driver_count*) ptr;
	if ( msg->device != ONE_AND_ONLY_DEVICE )
		return errno = EINVAL, -1;
	msg->driver_count = Video::GetNumDrivers();
	return 0;
}

static int GetDriverName(void* ptr, size_t size)
{
	if ( size != sizeof(struct dispmsg_get_driver_name) )
		return errno = EINVAL, -1;
	struct dispmsg_get_driver_name* msg =
		(struct dispmsg_get_driver_name* ) ptr;
	if ( msg->device != ONE_AND_ONLY_DEVICE )
		return errno = EINVAL, -1;
	char* name = Video::GetDriverName(msg->driver_index);
	if ( !name )
		return -1;
	bool success = TransmitString(&msg->name, name);
	delete[] name;
	return success ? 0 : -1;
}

static int GetDriver(void* ptr, size_t size)
{
	if ( size != sizeof(struct dispmsg_get_driver) )
		return errno = EINVAL, -1;
	struct dispmsg_get_driver* msg = (struct dispmsg_get_driver*) ptr;
	if ( msg->device != ONE_AND_ONLY_DEVICE )
		return errno = EINVAL, -1;
	msg->driver_index = Video::GetCurrentDriverIndex();
	return 0;
}

static int SetDriver(void* ptr, size_t size)
{
	if ( size != sizeof(struct dispmsg_set_driver) )
		return errno = EINVAL, -1;
	struct dispmsg_set_driver* msg = (struct dispmsg_set_driver*) ptr;
	if ( msg->device != ONE_AND_ONLY_DEVICE )
		return errno = EINVAL, -1;
	return errno = ENOSYS, -1;
}

static int SetCrtcMode(void* ptr, size_t size)
{
	if ( size != sizeof(struct dispmsg_set_crtc_mode) )
		return errno = EINVAL, -1;
	struct dispmsg_set_crtc_mode* msg = (struct dispmsg_set_crtc_mode*) ptr;
	if ( msg->device != ONE_AND_ONLY_DEVICE )
		return errno = EINVAL, -1;
	if ( msg->connector != ONE_AND_ONLY_CONNECTOR )
		return errno = EINVAL, -1;
	char* modestr = StringOfCrtcMode(msg->mode);
	if ( !modestr )
		return -1;
	bool success = Video::SwitchMode(modestr);
	delete[] modestr;
	return success ? 0 : -1;
}

static int GetCrtcMode(void* ptr, size_t size)
{
	if ( size != sizeof(struct dispmsg_get_crtc_mode) )
		return errno = EINVAL, -1;
	struct dispmsg_get_crtc_mode* msg = (struct dispmsg_get_crtc_mode*) ptr;
	if ( msg->device != ONE_AND_ONLY_DEVICE )
		return errno = EINVAL, -1;
	if ( msg->connector != ONE_AND_ONLY_CONNECTOR )
		return errno = EINVAL, -1;
	char* modestr = Video::GetCurrentMode();
	if ( !modestr )
		return -1;
	msg->mode = CrtcModeOfString(modestr, Video::GetCurrentDriverIndex());
	delete[] modestr;
	return 0;
}

static int GetCrtcModes(void* ptr, size_t size)
{
	if ( size != sizeof(struct dispmsg_get_crtc_modes) )
		return errno = EINVAL, -1;
	struct dispmsg_get_crtc_modes* msg = (struct dispmsg_get_crtc_modes*) ptr;
	if ( msg->device != ONE_AND_ONLY_DEVICE )
		return errno = EINVAL, -1;
	if ( msg->connector != ONE_AND_ONLY_CONNECTOR )
		return errno = EINVAL, -1;
	size_t nummodes;
	char** modes = Video::GetModes(&nummodes);
	if ( !modes )
		return -1;
	size_t dest_length = msg->modes_length;

	int ret;
	if ( nummodes <= dest_length )
	{
		ret = 0;
		for ( size_t i = 0; i < nummodes; i++ )
		{
			const char* modestr = modes[i];
			uint64_t driver_index = Video::LookupDriverIndexOfMode(modestr);
			msg->modes[i] = CrtcModeOfString(modestr, driver_index);
		}
	}
	else
	{
		errno = ERANGE;
		ret = -1;
	}
	msg->modes_length = nummodes;
	for ( size_t i = 0; i < nummodes; i++ )
		delete[] modes[i];
	delete[] modes;
	return ret;
}

static int GetMemorySize(void* ptr, size_t size)
{
	if ( size != sizeof(struct dispmsg_get_memory_size) )
		return errno = EINVAL, -1;
	struct dispmsg_get_memory_size* msg =
		(struct dispmsg_get_memory_size*) ptr;
	msg->memory_size = Video::FrameSize();
	return 0;
}

static int WriteMemory(void* ptr, size_t size)
{
	if ( size != sizeof(struct dispmsg_write_memory) )
		return errno = EINVAL, -1;
	struct dispmsg_write_memory* msg = (struct dispmsg_write_memory*) ptr;
	if ( msg->device != ONE_AND_ONLY_DEVICE )
		return errno = EINVAL, -1;
	if ( OFF_MAX < msg->offset )
		return errno = EOVERFLOW, -1;
	off_t offset = msg->offset;
	if ( Video::WriteAt(offset, msg->src, msg->size) < 0 )
		return -1;
	return 0;
}

static int ReadMemory(void* ptr, size_t size)
{
	if ( size != sizeof(struct dispmsg_read_memory) )
		return errno = EINVAL, -1;
	struct dispmsg_read_memory* msg = (struct dispmsg_read_memory*) ptr;
	if ( msg->device != ONE_AND_ONLY_DEVICE )
		return errno = EINVAL, -1;
	if ( OFF_MAX < msg->offset )
		return errno = EOVERFLOW, -1;
	off_t offset = msg->offset;
	if ( Video::ReadAt(offset, msg->dst, msg->size) < 0 )
		return -1;
	return 0;
}

// TODO: Secure this system call against bad user-space pointers.
static int sys_dispmsg_issue(void* ptr, size_t size)
{
	struct dispmsg_header* hdr = (struct dispmsg_header*) ptr;
	if ( size < sizeof(*hdr) )
		return errno = EINVAL, -1;
	switch ( hdr->msgid )
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

void Init()
{
	Syscall::Register(SYSCALL_DISPMSG_ISSUE, (void*) sys_dispmsg_issue);
}

} // namespace DisplayMessage
} // namespace Sortix
