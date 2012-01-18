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
	class Device
	{
	public:
		static const unsigned STREAM = 0;
		static const unsigned BUFFER = 1;
		static const unsigned VGABUFFER = 2;
		static const unsigned FILESYSTEM = 3;
		static const unsigned DIRECTORY = 4;
		static const unsigned TTY = 5;

	public:
		Device();
		virtual ~Device();

	private:
		size_t refcount;

	public:
		void Refer();
		void Unref();

	public:
		virtual bool IsType(unsigned type) const = 0;

	};
}

#endif

