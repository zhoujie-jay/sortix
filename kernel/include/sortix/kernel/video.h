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

    sortix/kernel/video.h
    Framework for Sortix video drivers.

*******************************************************************************/

#ifndef INCLUDE_SORTIX_KERNEL_VIDEO_H
#define INCLUDE_SORTIX_KERNEL_VIDEO_H

#include <sys/types.h>

#include <sortix/display.h>

#include <sortix/kernel/ioctx.h>
#include <sortix/kernel/refcount.h>

namespace Sortix {

class TextBuffer;
class TextBufferHandle;

class VideoDevice
{
public:
	virtual ~VideoDevice() { }
	virtual struct dispmsg_crtc_mode GetCurrentMode(uint64_t connector) const = 0;
	virtual bool SwitchMode(uint64_t connector, struct dispmsg_crtc_mode mode) = 0;
	virtual bool Supports(uint64_t connector, struct dispmsg_crtc_mode mode) const = 0;
	virtual struct dispmsg_crtc_mode* GetModes(uint64_t connector, size_t* nummodes) const = 0;
	virtual off_t FrameSize() const = 0;
	virtual ssize_t WriteAt(ioctx_t* ctx, off_t off, const void* buf, size_t count) = 0;
	virtual ssize_t ReadAt(ioctx_t* ctx, off_t off, void* buf, size_t count) = 0;
	virtual TextBuffer* CreateTextBuffer(uint64_t connector) = 0;

};

} // namespace Sortix

namespace Sortix {
namespace Video {

void Init(Ref<TextBufferHandle> textbufhandle);
bool RegisterDevice(const char* name, VideoDevice* device);

} // namespace Video
} // namespace Sortix

#endif
