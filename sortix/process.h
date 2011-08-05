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

	process.h
	Describes a process belonging to a subsystem.

******************************************************************************/

#ifndef SORTIX_PROCESS_H
#define SORTIX_PROCESS_H

#include "descriptors.h"

namespace Sortix
{
	class Thread;
	class Process;
	class MountPoint;
	class User;
	class System;
	class DevBuffer;

	class Process
	{
	public:
		// TODO: Make this possible from an async syscall!
		//bool CreateProcess(System* system, DevBuffer* image);

	public:
		DescriptorTable _descs;

	private:
		MountPoint* _rootFS;
		User* _user;
		System* _system;

	public:
		MountPoint* GetRootFS() { _rootFS; }
		User* GetUser() { _user; }
		System* GetSystem() { _system; }

	private:
		Process* _parent;
		Process* _child;
		Process* _prevSibling;
		Process* _nextSibling;

	private:
		Thread* _firstThread;

	};
}

#endif

