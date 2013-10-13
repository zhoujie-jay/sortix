/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2012.

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

	linebuffer.h
	Provides a simple queue-like line buffering for terminals.

******************************************************************************/

#ifndef SORTIX_LINEBUFFER_H
#define SORTIX_LINEBUFFER_H

namespace Sortix
{
	class LineBuffer
	{
	public:
		LineBuffer();
		~LineBuffer();

	public:
		bool Push(uint32_t unicode);
		uint32_t Pop();
		uint32_t Peek() const;
		uint32_t Backspace();
		void Commit();
		void Freeze();
		bool CanPop() const;
		bool CanBackspace() const;

	private:
		uint32_t* buffer;
		size_t bufferlength;
		size_t bufferoffset;
		size_t buffercommitted;
		size_t bufferfrozen;
		size_t bufferused;

	};
}

#endif
