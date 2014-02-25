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

    sortix/kernel/video.h
    Framework for Sortix video drivers.

*******************************************************************************/

#ifndef SORTIX_VIDEO_H
#define SORTIX_VIDEO_H

#include <sys/types.h>

#include <sortix/kernel/refcount.h>

namespace Sortix {

class TextBuffer;
class TextBufferHandle;

bool ReadParamString(const char* str, ...);

class VideoDriver
{
public:
	virtual ~VideoDriver() { }
	virtual bool StartUp() = 0;
	virtual bool ShutDown() = 0;
	virtual char* GetCurrentMode() const = 0;
	virtual bool SwitchMode(const char* mode) = 0;
	virtual bool Supports(const char* mode) const = 0;
	virtual char** GetModes(size_t* nummodes) const = 0;
	virtual off_t FrameSize() const = 0;
	virtual ssize_t WriteAt(off_t off, const void* buf, size_t count) = 0;
	virtual ssize_t ReadAt(off_t off, void* buf, size_t count) = 0;
	virtual TextBuffer* CreateTextBuffer() = 0;

};

namespace Video {

void Init(Ref<TextBufferHandle> textbufhandle);
bool RegisterDriver(const char* name, VideoDriver* driver);
char* GetCurrentMode();
char* GetDriverName(size_t index);
size_t GetCurrentDriverIndex();
size_t GetNumDrivers();
size_t LookupDriverIndexOfMode(const char* mode);
bool Supports(const char* mode);
bool SwitchMode(const char* mode);
char** GetModes(size_t* modesnum);
off_t FrameSize();
ssize_t WriteAt(off_t off, const void* buf, size_t count);
ssize_t ReadAt(off_t off, void* buf, size_t count);

} // namespace Video

} // namespace Sortix

#endif
