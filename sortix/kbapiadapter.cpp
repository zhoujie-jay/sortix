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

	kbapidapter.cpp
	This class is a hack connecting the Keyboard and Keyboard layout classes
	with the older and really bad Keyboard API. This class is intended to be
	replaced by a real terminal driver working over /dev/tty descriptors.

******************************************************************************/

#include "platform.h"
#include <libmaxsi/keyboard.h>
#include "scheduler.h" // SIGINT
#include "keyboard.h"
#include "keycodes.h"
#include "kbapiadapter.h"

namespace Sortix
{
	KBAPIAdapter::KBAPIAdapter(Keyboard* keyboard, KeyboardLayout* kblayout)
	{
		this->keyboard = keyboard;
		this->kblayout = kblayout;
		this->control = false;
		this->queuelength = QUEUELENGTH;
		this->queueused = 0;
		this->queueoffset = 0;
		keyboard->SetOwner(this, NULL);
	}

	KBAPIAdapter::~KBAPIAdapter()
	{
		delete keyboard;
		delete kblayout;
	}

	void KBAPIAdapter::OnKeystroke(Keyboard* kb, void* /*user*/)
	{
		while ( kb->HasPending() )
		{
			ProcessKeystroke(kb->Read());
		}
	}

	void KBAPIAdapter::ProcessKeystroke(int kbkey)
	{
		if ( !kbkey ) { return; }
		int abskbkey = (kbkey < 0) ? -kbkey : kbkey;

		uint32_t unicode = kblayout->Translate(kbkey);

		// Now translate the keystroke into the older API's charset..
		uint32_t maxsicode = (unicode) ? unicode : ToMaxsiCode(abskbkey);

		if ( kbkey < 0 ) { maxsicode |= Maxsi::Keyboard::DEPRESSED; }

		if ( kbkey == KBKEY_LCTRL ) { control = true; }
		if ( kbkey == -KBKEY_LCTRL ) { control = false; }
		if ( control && kbkey == KBKEY_C )
		{
			Scheduler::SigIntHack();
			return;
		}

		if ( !QueueKeystroke(maxsicode) )
		{
			Log::PrintF("Warning: KBAPIAdapter driver dropping keystroke due "
			            "to insufficient buffer space\n");
		}
	}
	
	uint32_t KBAPIAdapter::ToMaxsiCode(int abskbkey)
	{
		switch ( abskbkey )
		{
		case KBKEY_ESC: return Maxsi::Keyboard::ESC;
		case KBKEY_LCTRL: return Maxsi::Keyboard::CTRL;
		case KBKEY_LSHIFT: return Maxsi::Keyboard::LSHFT;
		case KBKEY_RSHIFT: return Maxsi::Keyboard::RSHFT;
		case KBKEY_LALT: return Maxsi::Keyboard::ALT;
		case KBKEY_F1: return Maxsi::Keyboard::F1;
		case KBKEY_F2: return Maxsi::Keyboard::F2;
		case KBKEY_F3: return Maxsi::Keyboard::F3;
		case KBKEY_F4: return Maxsi::Keyboard::F4;
		case KBKEY_F5: return Maxsi::Keyboard::F5;
		case KBKEY_F6: return Maxsi::Keyboard::F6;
		case KBKEY_F7: return Maxsi::Keyboard::F7;
		case KBKEY_F8: return Maxsi::Keyboard::F8;
		case KBKEY_F9: return Maxsi::Keyboard::F9;
		case KBKEY_F10: return Maxsi::Keyboard::F10;
		case KBKEY_F11: return Maxsi::Keyboard::F11;
		case KBKEY_F12: return Maxsi::Keyboard::F12;
		case KBKEY_SCROLLLOCK: return Maxsi::Keyboard::SCRLCK;
		case KBKEY_HOME: return Maxsi::Keyboard::HOME;
		case KBKEY_UP: return Maxsi::Keyboard::UP;
		case KBKEY_LEFT: return Maxsi::Keyboard::LEFT;
		case KBKEY_RIGHT: return Maxsi::Keyboard::RIGHT;
		case KBKEY_DOWN: return Maxsi::Keyboard::DOWN;
		case KBKEY_PGUP: return Maxsi::Keyboard::PGUP;
		case KBKEY_PGDOWN: return Maxsi::Keyboard::PGDOWN;
		case KBKEY_END: return Maxsi::Keyboard::END;
		case KBKEY_INSERT: return Maxsi::Keyboard::INS;
		case KBKEY_DELETE: return Maxsi::Keyboard::DEL;
		case KBKEY_CAPSLOCK: return Maxsi::Keyboard::CAPS;
		case KBKEY_RALT: return Maxsi::Keyboard::ALTGR;
		case KBKEY_NUMLOCK: return Maxsi::Keyboard::NUMLCK;
		default: return Maxsi::Keyboard::UNKNOWN;
		}
	}

	bool KBAPIAdapter::QueueKeystroke(uint32_t keystroke)
	{
		if ( queuelength <= queueused ) { return false; }		
		queue[(queueoffset + queueused++) % queuelength] = keystroke;
		return true;
	}

	uint32_t KBAPIAdapter::DequeueKeystroke()
	{
		if ( !queueused ) { return 0; }
		uint32_t codepoint = queue[queueoffset++];
		queueoffset %= queuelength;
		queueused--;
		return codepoint;
	}
}
