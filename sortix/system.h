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

	system.h
	Describes a subsystem, thus allowing multiple OSes to run on Sortix.

******************************************************************************/

#ifndef SORTIX_SYSTEM_H
#define SORTIX_SYSTEM_H

namespace Sortix
{
	class Process;
	class Authorization;

	class System
	{
	private:
		int _systemId;
		Process* _init;
		Authorization* _authorization;

	private:
		// TODO: a list of processes belonging to this subsystem.

	private:
		int _baseNiceness;
		// TODO: stuff like memory limits, etc.

	};
}

#endif

