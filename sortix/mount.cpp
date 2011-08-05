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

	mount.cpp
	Handles system wide mount points and initialization of new file systems.

******************************************************************************/

#include "platform.h"
#include <libmaxsi/string.h>
#include "device.h"
#include "filesystem.h"
#include "thread.h"
#include "process.h"
#include "memorymanagement.h"

namespace Sortix
{
	namespace Syscall
	{
		void SysOpen(Thread* thread)
		{
			struct Parameters
			{
				struct
				{
					const char* USER userPath SYSPARAM;
					nat openFlags SYSPARAM;
					nat mode SYSPARAM;
				};
				struct
				{
					int handle;
					bool pending;
					FileSystem::SysCallback callbackInfo;
					const char* safePath;
				};
			};

			// We store our parameters and data inside the thread object, and we
			// are passed parameters in a special way, so make sure it works.
			STATIC_ASSERT(0 * sizeof(size_t) == offsetof(Parameters, userPath));
			STATIC_ASSERT(1 * sizeof(size_t) == offsetof(Parameters, openflags));
			STATIC_ASSERT(2 * sizeof(size_t) == offsetof(Parameters, mode));
			STATIC_ASSERT(sizeof(Parameters) <= sizeof(thread->sysParams));
			Parameters* params = (Parameters*) thread->sysParams;

			int handle;
			const char* path;

			// Initialize our variables and validate parameters.
			if ( !thread->sysParamsInited )
			{
				thread->sysParamsInited = true;
				params->handle = handle = -1;
				params->pending = false;

				// Make sure the path is a valid string in userspace.
				params->safePath = path = ValidateUserString(params->userPath)
				if ( path == NULL ) { thread->SysReturnError(-1); return; }

				params->openFlags &= O_USERSPACEABLE;
			}
			else
			{
				path = params->safePath;
				handle = param->handle;
			}

			// Get a descriptor for our device.
			if ( handle == -1 )
			{
				param->handle = handle = thread->GetProcess()->_descs.Reserve();
				if ( handle < 0 ) { thread->SysReturnError(handle); return; }
			}

			// Attempt to open the path. If the operation is blocking, this
			// function will re-called when it completes, successful or not.
			if ( !params->pending )
			{
				params->pending = true;

				MountPoint* mount = Mount::GetMountPoint(path);
				if ( mount == NULL ) { Error::Set(Error::BADINPUT); thread->SysReturnError(-1); return; }

				if ( !mount->fs->Open(path, params->openFlags, params->mode, &params->callbackInfo, thread) ) { return; }
			}

			// Return an error if the open failed.
			if ( unlikely(params->callbackInfo.device == NULL) )
			{
				handle = thread->GetProcess()->_descs.Free(handle);
				Error::Set(params->callbackInfo.deviceError);
				thread->SysReturnError(-1); return;
			}

			// Bind the device to the descriptor and return it.
			handle = thread->GetProcess()->_descs.UseReservation(handle, params->callbackInfo.device);
			thread->SysReturn(handle); return;
		}

		void SysMount(Thread* thread)
		{
			struct Parameters
			{
				struct
				{
					const char* USER userDevicePath SYSPARAM;
					const char* USER userTargetPath SYSPARAM;
					const char* USER userDriverName SYSPARAM;
					nat mode SYSPARAM;
				};
				struct
				{
					int stage;
					DevBuffer* storage;
					DevFileSystem* fsDriver;
					DevDirectory* directory;
					FileSystem::SysCallback callbackInfo;
				};
			};

			// We store our parameters and data inside the thread object, and we
			// are passed parameters in a special way, so make sure it works.
			STATIC_ASSERT(0 * sizeof(size_t) == offsetof(Parameters, userDevicePath));
			STATIC_ASSERT(1 * sizeof(size_t) == offsetof(Parameters, userTargetPath));
			STATIC_ASSERT(2 * sizeof(size_t) == offsetof(Parameters, userDriverName));
			STATIC_ASSERT(3 * sizeof(size_t) == offsetof(Parameters, mode));
			STATIC_ASSERT(sizeof(Parameters) <= sizeof(thread->sysParams));
			Parameters* params = (Parameters*) thread->sysParams;

			// Initialize our variables and validate parameters.
			if ( !thread->sysParamsInited )
			{
				thread->sysParamsInited = true;
				if ( ValidateUserString(params->userDevicePath) == NULL ||
				     ValidateUserString(params->userTargetPath) == NULL ||
				     ( params->userDriverName != NULL &&
				       ValidateUserString(params->userDriverName) == NULL ) )
				{
					thread->SysReturnError(-1); return;
				}

				params->mode &= O_USERSPACEABLE;
				nat baseMode = params->mode & O_LOWERFLAGS;
				nat extMode = params->mode & (~O_LOWERFLAGS);

				// Deny unsupported open modes.
				if ( (baseMode != O_RDWR && baseMode != O_RDONLY) ||
				     (extMode != 0 && extMode != O_EXCLUSIVELY && extMode != O_SYNC) )
				{
					Error::Set(Error::BADINPUT);
					thread->SysReturnError(-1); return;
				}

				params->stage = 0;
				params->storage = NULL;
				params->fsDriver = NULL;
				params->directory = NULL;
			}

			MountPoint* mount;
			Device* device = NULL;

			switch ( params->stage )
			{
			// Get a handle to the storage device.
			case 0: params->stage++;

				mount = Mount::GetMountPoint(&params->userDevicePath);
				if ( mount == NULL ) { Error::Set(Error::BADINPUT); break; }

				ASSERT(params->mode & O_USERSPACEABLE == 0);
				if ( !mount->fs->Open(params->userDevicePath, params->mode | O_EXCLUSIVELY, 0, &params->callbackInfo, thread) ) { return; }
			
			// Validate the properness of the storage device.
			case 1: params->stage++;
				device = params->callbackInfo.device;

				// Did it exist/could we open it/did we have permission?
				if ( unlikely(device == NULL) ) { Error::Set(params->callbackInfo.deviceError); break; }

				// Is it a buffer?
				if ( unlikely(!device->IsType(Device::BUFFER)) ) { Error::Set(Error::ENOTBLK); break; }

				params->storage = (DevBuffer*) device; device = NULL;

			// Get a handle to the destination directory.
			case 2: params->stage++;

				mount = Mount::GetMountPoint(&params->userTargetPath);
				if ( mount == NULL ) { Error::Set(Error::BADINPUT); break; }

				// TODO: Figure out some good mode flags here!
				// TODO: Make sure the FS driver forces the dest dir to be empty!
				if ( !mount->fs->Open(params->userTargetPath, O_RDWR | O_DIRECTORY | O_MOUNT, 0, &params->callbackInfo, thread) ) { return; }
			
			// Validate the properness of the destination directory.
			case 2: params->stage++;
				device = params->callbackInfo.device;

				// Did it exist/could we open it?
				if ( unlikely(device == NULL) ) { Error::Set(params->callbackInfo.deviceError); break; }

				// Is it a directory?
				if ( unlikely(!device->IsType(Device::DIRECTORY)) ) { Error::Set(Error::ENOTDIR); break; }

				params->directory = (DevDirectory*) device; device = NULL;

				// Validate that the user owns the directory.
				if ( !params->directory->IsOwner(thread->GetProcess()->GetUser()) ) { Error::Set(Error::EPERM); break; }

				// TODO: Add a run-time assertion that the dir is empty!

			// Load the proper driver for this file system type.
			case 3: params->stage++;

				// TODO: Make an async interface that can scan a device!
				params->fsDriver = FileSystem::CreateDriver(params->userDriverName);
				if ( params->fsDriver == NULL ) { Error::Set(Error::ENODEV); break; }

			// Now bind the file system driver to the device and directory.
			case 4: params->stage++;
				MountPoint* newMount = new MountPoint();

				newMount->device = params->storage;
				newMount->fs = params->fsDriver;

				// TODO: Actually do the mounting!

			// Check if everything went well.
			case 4:
				// TODO: Check if everything went well!
				thread->SysReturn(0); return;
			}

			if ( device != NULL ) { device->close(); }
			if ( params->storage != NULL ) { params->storage->close(); }
			if ( params->directory != NULL ) { params->directory->close(); }
			if ( params->fsDriver != NULL ) { params->fsDriver->close(); }

			thread->SysReturnError(-1); return;
		}
	}

	namespace Mount
	{
		MountPoint* GetMountPoint(const char** path, Thread* thread)
		{
			ASSERT(path != NULL);
			ASSERT(*path != NULL);
			ASSERT(thread != NULL);

			if ( unlikely(**path != '/') ) { return NULL; } (*path)++;	
			MountPoint* currentFS = thread->GetProcess->GetRootFS();

			ASSERT(currentFS != NULL);

			// Simply check if path belongs to a child mount point.
			for ( MountPoint* considering = currentFS->child; considering != NULL; considering = considering->nextSibling )
			{
				ASSERT(considering->IsSane());

				size_t consideringPathLen = String::Length(considering->path);
				if ( String::CompareN(*path, considering->path, consideringPathLen) == 0 )
				{
					// Check if this is the right dir!
					char lastChar = (*path)[consideringPathLen];

					if ( lastChar == '/' ) { (*path) += consideringPathLen + 1; }
					else if ( lastChar == '\0' ) { (*path) += consideringPathLen + 0; }
					else { continue; }

					// Check if the mountpoint isn't ready.
					if ( considering->waiting != NULL ) { Error::Set(Error::EBUSY); return NULL; }

					currentFS = considering;
					considering = currentFS->child;
				}
			}

			ASSERT(currentFS->fs != NULL);
		
			return currentFS;
		}
	}
}


