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

	kbapidapter.h
	This class is a hack connecting the Keyboard and Keyboard layout classes
	with the older and really bad Keyboard API. This class is intended to be
	replaced by a real terminal driver working over /dev/tty descriptors.

******************************************************************************/

#ifndef SORTIX_KBAPIADAPTER_H
#define SORTIX_KBAPIADAPTER_H

#include "keyboard.h"

namespace Sortix
{
	class KBAPIAdapter : public KeyboardOwner
	{
	public:
		KBAPIAdapter(Keyboard* keyboard, KeyboardLayout* kblayout);
		virtual ~KBAPIAdapter();
		virtual void OnKeystroke(Keyboard* keyboard, void* user);

	public:
		uint32_t DequeueKeystroke();

	private:
		void ProcessKeystroke(int kbkey);
		bool QueueKeystroke(uint32_t keystroke);
		static uint32_t ToMaxsiCode(int abskbkey);

	private:
		Keyboard* keyboard;
		KeyboardLayout* kblayout;
		bool control;
		static const size_t QUEUELENGTH = 1024UL;
		size_t queuelength;
		size_t queueused;
		size_t queueoffset;
		uint32_t queue[QUEUELENGTH];

	};
}

#endif

