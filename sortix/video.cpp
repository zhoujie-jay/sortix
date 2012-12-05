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

	video.cpp
	Framework for Sortix video drivers.

*******************************************************************************/

#include <sortix/kernel/platform.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/textbuffer.h>
#include <sortix/kernel/video.h>
#include <sortix/kernel/string.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>

namespace Sortix {

bool ReadParamString(const char* str, ...)
{
	if ( strchr(str, '\n') ) { errno = EINVAL; }
	const char* keyname;
	va_list args;
	while ( *str )
	{
		size_t varlen = strcspn(str, ",");
		if ( !varlen ) { str++; continue; }
		size_t namelen = strcspn(str, "=");
		if ( !namelen ) { errno = EINVAL; goto cleanup; }
		if ( !str[namelen] ) { errno = EINVAL; goto cleanup; }
		if ( varlen < namelen ) { errno = EINVAL; goto cleanup; }
		size_t valuelen = varlen - 1 /*=*/ - namelen;
		char* name = String::Substring(str, 0, namelen);
		if ( !name ) { goto cleanup; }
		char* value = String::Substring(str, namelen+1, valuelen);
		if ( !value ) { delete[] name; goto cleanup; }
		va_start(args, str);
		while ( (keyname = va_arg(args, const char*)) )
		{
			if ( strcmp(keyname, "STOP") == 0 )
				break;
			char** nameptr = va_arg(args, char**);
			if ( strcmp(keyname, name) ) { continue; }
			*nameptr = value;
			break;
		}
		va_end(args);
		if ( !keyname ) { delete[] value; }
		delete[] name;
		str += varlen;
		str += strspn(str, ",");
	}
	return true;

cleanup:
	va_start(args, str);
	while ( (keyname = va_arg(args, const char*)) )
	{
		char** nameptr = va_arg(args, char**);
		delete[] *nameptr; *nameptr = NULL;
	}
	va_end(args);
	return false;
}

namespace Video {

const unsigned long DRIVER_GOT_MODES = (1UL<<0UL);

struct DriverEntry
{
	char* name;
	VideoDriver* driver;
	unsigned long flags;
	size_t id;
};

size_t numdrivers;
size_t driverslen;
DriverEntry* drivers;

size_t nummodes;
size_t modeslen;
char** modes;

char* currentmode;
size_t currentdrvid;
bool newdrivers;

kthread_mutex_t videolock;
TextBufferHandle* textbufhandle;

void Init(TextBufferHandle* thetextbufhandle)
{
	videolock = KTHREAD_MUTEX_INITIALIZER;
	textbufhandle = thetextbufhandle;
	numdrivers = driverslen = 0;
	drivers = NULL;
	nummodes = modeslen = 0;
	modes = NULL;
	currentdrvid = SIZE_MAX;
	newdrivers = false;
	currentmode = NULL;
}

static DriverEntry* CurrentDriverEntry()
{
	if ( currentdrvid == SIZE_MAX ) { return NULL; }
	return drivers + currentdrvid;
}

bool RegisterDriver(const char* name, VideoDriver* driver)
{
	ScopedLock lock(&videolock);
	if ( numdrivers == driverslen )
	{
		size_t newdriverslen = driverslen ? 2 * driverslen : 8UL;
		DriverEntry* newdrivers = new DriverEntry[newdriverslen];
		if ( !newdrivers ) { return false; }
		memcpy(newdrivers, drivers, sizeof(*drivers) * numdrivers);
		delete[] drivers; drivers = newdrivers;
		driverslen = driverslen;
	}

	char* drivername = String::Clone(name);
	if ( !drivername ) { return false; }

	size_t index = numdrivers++;
	drivers[index].name = drivername;
	drivers[index].driver = driver;
	drivers[index].flags = 0;
	drivers[index].id = index;
	newdrivers = true;
	return true;
}

static bool ExpandModesArray(size_t needed)
{
	size_t modesneeded = nummodes + needed;
	if ( modesneeded <= modeslen ) { return true; }
	size_t newmodeslen = 2 * modeslen;
	if ( newmodeslen < modesneeded ) { newmodeslen = modesneeded; }
	char** newmodes = new char*[newmodeslen];
	if ( !newmodes ) { return false; }
	memcpy(newmodes, modes, sizeof(char*) * nummodes);
	delete[] modes; modes = newmodes;
	modeslen = newmodeslen;
	return true;
}

static void UpdateModes()
{
	if ( !newdrivers ) { return; }
	bool allsuccess = true;
	for ( size_t i = 0; i < numdrivers; i++ )
	{
		bool success = false;
		if ( drivers[i].flags & DRIVER_GOT_MODES ) { continue; }
		const char* drivername = drivers[i].name;
		VideoDriver* driver = drivers[i].driver;
		size_t prevnummodes = nummodes;
		size_t drvnummodes = 0;
		char** drvmodes = driver->GetModes(&drvnummodes);
		if ( !drvmodes ) { goto cleanup_error; }
		if ( !ExpandModesArray(drvnummodes) ) { goto cleanup_drvmodes; }
		for ( size_t n = 0; n < drvnummodes; n++ )
		{
			char* modestr = String::Combine(4, "driver=", drivername,
			                                   ",", drvmodes[n]);
			if ( !modestr ) { goto cleanup_newmodes; }
			modes[nummodes++] = modestr;
		}
		success = true;
		drivers[i].flags |= DRIVER_GOT_MODES;
cleanup_newmodes:
		for ( size_t i = prevnummodes; !success && i < nummodes; i++ )
			delete[] modes[i];
		if ( !success ) { nummodes = prevnummodes; }
cleanup_drvmodes:
		for ( size_t n = 0; n < drvnummodes; n++ ) { delete[] drvmodes[n]; }
		delete[] drvmodes;
cleanup_error:
		allsuccess &= success;
	}
	newdrivers = !allsuccess;
}

static DriverEntry* GetDriverEntry(const char* drivername)
{
	for ( size_t i = 0; i < numdrivers; i++ )
	{
		if ( !strcmp(drivername, drivers[i].name) )
		{
			return drivers + i;
		}
	}
	errno = ENODRV;
	return NULL;
}

static bool StartUpDriver(VideoDriver* driver, const char* drivername)
{
	if ( !driver->StartUp() )
	{
		int errnum = errno;
		Log::PrintF("Error: Video driver '%s' was unable to startup\n",
		            drivername);
		errno = errnum;
		return false;
	}
	return true;
}

static bool ShutDownDriver(VideoDriver* driver, const char* drivername)
{
	textbufhandle->Replace(NULL);
	if ( !driver->ShutDown() )
	{
		int errnum = errno;
		Log::PrintF("Warning: Video driver '%s' did not shutdown cleanly\n",
		            drivername);
		errno = errnum;
		return false;
	}
	return true;
}

static bool DriverModeAction(VideoDriver* driver, const char* drivername,
                             const char* mode, const char* action)
{
	textbufhandle->Replace(NULL);
	if ( !driver->SwitchMode(mode) )
	{
		int errnum = errno;
		Log::PrintF("Error: Video driver '%s' could not %s mode '%s'\n",
		            drivername, action, mode);
		errno = errnum;
		return false;
	}
	textbufhandle->Replace(driver->CreateTextBuffer());
	return true;
}

static bool SwitchDriverMode(VideoDriver* driver, const char* drivername,
                             const char* mode)
{
	return DriverModeAction(driver, drivername, mode, "switch to");
}


static bool RestoreDriverMode(VideoDriver* driver, const char* drivername,
                             const char* mode)
{
	return DriverModeAction(driver, drivername, mode, "restore");
}

// Attempts to use the specific driver and mode, if an error occurs, it will
// attempt to reload the previous driver and mode. If that fails, we are kinda
// screwed and the video adapter is left in an undefined state.
static bool DoSwitchMode(DriverEntry* newdrvent, const char* newmode)
{
	DriverEntry* prevdrvent = CurrentDriverEntry();
	VideoDriver* prevdriver = prevdrvent ? prevdrvent->driver : NULL;
	const char* prevdrivername = prevdrvent ? prevdrvent->name : NULL;

	VideoDriver* newdriver = newdrvent->driver;
	const char* newdrivername = newdrvent->name;

	char* newcurrentmode = String::Clone(newmode);
	if ( !newcurrentmode ) { return false; }

	if ( prevdriver == newdriver )
	{
		if ( !SwitchDriverMode(newdriver, newdrivername, newmode) )
		{
			delete[] newcurrentmode;
			return false;
		}
		delete[] currentmode;
		currentmode = newcurrentmode;
		return true;
	}

	int errnum = 0;

	if ( prevdriver ) { ShutDownDriver(prevdriver, prevdrivername); }

	char* prevmode = currentmode; currentmode = NULL;
	currentdrvid = SIZE_MAX;

	if ( !StartUpDriver(newdriver, newdrivername) )
	{
		errnum = errno;
		goto restore_prev_driver;
	}

	currentdrvid = newdrvent->id;

	if ( !SwitchDriverMode(newdriver, newdrivername, newmode) )
	{
		errnum = errno;
		ShutDownDriver(newdriver, newdrivername);
		currentdrvid = SIZE_MAX;
		goto restore_prev_driver;
	}

	currentmode = newcurrentmode;
	delete[] prevmode;

	return true;

restore_prev_driver:
	delete[] newcurrentmode;
	if ( !prevdriver ) { goto error_out; }
	if ( !StartUpDriver(prevdriver, prevdrivername) ) { goto error_out; }

	currentdrvid = prevdrvent->id;

	if ( !RestoreDriverMode(prevdriver, prevdrivername, prevmode) )
	{
		ShutDownDriver(prevdriver, prevdrivername);
		currentdrvid = SIZE_MAX;
		goto error_out;
	}

	Log::PrintF("Successfully restored video driver '%s' mode '%s'\n",
	            prevdrivername, prevmode);

error_out:
	if ( currentdrvid == SIZE_MAX )
		Log::PrintF("Warning: Could not fall back upon a video driver\n");
	errno = errnum; // Return the original error, not the last one.
	return false;
}

char* GetCurrentMode()
{
	ScopedLock lock(&videolock);
	UpdateModes();
	return String::Clone(currentmode ? currentmode : "driver=none");
}

char** GetModes(size_t* modesnum)
{
	ScopedLock lock(&videolock);
	UpdateModes();
	char** result = new char*[nummodes];
	if ( !result ) { return NULL; }
	for ( size_t i = 0; i < nummodes; i++ )
	{
		result[i] = String::Clone(modes[i]);
		if ( !result[i] )
		{
			for ( size_t j = 0; j < i; j++ )
			{
				delete[] result[j];
			}
			delete[] result;
			return NULL;
		}
	}
	*modesnum = nummodes;
	return result;
}

bool SwitchMode(const char* mode)
{
	ScopedLock lock(&videolock);
	UpdateModes();
	char* drivername = NULL;
	if ( !ReadParamString(mode, "driver", &drivername, NULL) ) { return false; }
	if ( !strcmp(drivername, "none") )
	{
		DriverEntry* driverentry = CurrentDriverEntry();
		if ( !driverentry )
			return true;
		ShutDownDriver(driverentry->driver, driverentry->name);
		currentdrvid = SIZE_MAX;
		return true;
	}
	DriverEntry* drvent = GetDriverEntry(drivername);
	delete[] drivername;
	if ( !drvent ) { return false; }
	return DoSwitchMode(drvent, mode);
}

bool Supports(const char* mode)
{
	ScopedLock lock(&videolock);
	UpdateModes();
	char* drivername = NULL;
	if ( !ReadParamString(mode, "driver", &drivername, NULL) ) { return false; }
	DriverEntry* drvent = GetDriverEntry(drivername);
	delete[] drivername;
	if ( !drvent ) { return false; }
	return drvent->driver->Supports(mode);
}

off_t FrameSize()
{
	ScopedLock lock(&videolock);
	DriverEntry* drvent = CurrentDriverEntry();
	if ( !drvent ) { errno = EINVAL; return -1; }
	return drvent->driver->FrameSize();
}

ssize_t WriteAt(off_t off, const void* buf, size_t count)
{
	ScopedLock lock(&videolock);
	DriverEntry* drvent = CurrentDriverEntry();
	if ( !drvent ) { errno = EINVAL; return -1; }
	return drvent->driver->WriteAt(off, buf, count);
}

ssize_t ReadAt(off_t off, void* buf, size_t count)
{
	ScopedLock lock(&videolock);
	DriverEntry* drvent = CurrentDriverEntry();
	if ( !drvent ) { errno = EINVAL; return -1; }
	return drvent->driver->ReadAt(off, buf, count);
}

} // namespace Video

} // namespace Sortix
