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

	device.h
	A base class for all devices.

******************************************************************************/

#ifndef SORTIX_DEVICE_H
#define SORTIX_DEVICE_H

namespace Sortix
{
	class User;

	class Device
	{
	public:
		// Flags
		static const nat READABLE = (1<<0);
		static const nat WRITABLE = (1<<1);
		static const nat SEEKABLE = (1<<2);
		static const nat SIZEABLE = (1<<3);
		static const nat BLOCK = (1<<4);
		static const nat FLAGMASK = ((1<<5)-1);

		// Types
		static const nat TYPEMASK = ~FLAGMASK;
		static const nat STREAM = (1<<5);
		static const nat BUFFER = (2<<5);
		static const nat DIRECTORY = (3<<5);
		static const nat FILESYSTEM = (4<<5);
		static const nat NETWORK = (5<<5);
		static const nat SOUND = (6<<5);
		static const nat GRAPHICS = (7<<5);
		static const nat MOUSE = (8<<5);
		static const nat KEYBOARD = (9<<5);
		static const nat PRINTER = (10<<5);
		static const nat SCANNER = (11<<5);
		static const nat VGABUFFER = (12<<5);
		static const nat OTHER = TYPEMASK;

	public:
		volatile size_t _refCount;

	public:
		Device() { _refCount = 1; }
		virtual ~Device() { }

	private:
		User* _owner;

	public:
		bool IsOwner(User* candidate) { return candidate == _owner; }

	public:
		bool Close();
		void RequestThink();

	protected:
		virtual void Think();

	public:
		virtual bool Sync();
		virtual nat Flags() = 0;

	public:
		bool IsType(nat type) { return (Flags() & TYPEMASK) == type; } 

	};
}

#endif

