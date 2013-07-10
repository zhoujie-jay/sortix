/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012.

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

    keyboard.h
    An interface to keyboards.

*******************************************************************************/

#ifndef SORTIX_KEYBOARD_H
#define SORTIX_KEYBOARD_H

namespace Sortix
{
	class Keyboard;
	class KeyboardOwner;

	class Keyboard
	{
	public:
		static void Init();

	public:
		virtual ~Keyboard() { }
		virtual int Read() = 0;
		virtual size_t GetPending() const = 0;
		virtual bool HasPending() const = 0;
		virtual void SetOwner(KeyboardOwner* owner, void* user) = 0;
	};

	class KeyboardOwner
	{
	public:
		virtual ~KeyboardOwner() { }
		virtual void OnKeystroke(Keyboard* keyboard, void* user) = 0;
	};

	class KeyboardLayout
	{
	public:
		virtual ~KeyboardLayout() { }
		virtual uint32_t Translate(int kbkey) = 0;
	};
}

#endif
