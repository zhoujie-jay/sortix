/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011, 2012.

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

	ks/layout/us.cpp
	The United States keyboard layout.

******************************************************************************/

#include <sortix/kernel/platform.h>
#include "../../keyboard.h"
#include <sortix/keycodes.h>
#include "us.h"

namespace Sortix
{
	const unsigned MOD_SHIFT = (1U<<0U);
	const unsigned MOD_CAPS = (1U<<1U);
	const unsigned MOD_LSHIFT = (1U<<2U);
	const unsigned MOD_RSHIFT = (1U<<3U);

	const uint32_t LAYOUT_US[4UL*128UL] =
	{
		0, 0, 0, 0, /* unused: kbkey 0 is invalid */
		0, 0, 0, 0, /* KBKEY_ESC */
		'1', '!', '1', '!',
		'2', '@', '2', '@',
		'3', '#', '3', '#',
		'4', '$', '4', '$',
		'5', '%', '5', '%',
		'6', '^', '6', '^',
		'7', '&', '7', '&',
		'8', '*', '8', '*',
		'9', '(', '9', '(',
		'0', ')', '0', ')',
		'-', '_', '-', '_',
		'=', '+', '=', '+',
		'\b', '\b', '\b', '\b',
		'\t', '\t', '\t', '\t',
		'q', 'Q', 'Q', 'q',
		'w', 'W', 'W', 'w',
		'e', 'E', 'E', 'e',
		'r', 'R', 'R', 'r',
		't', 'T', 'T', 't',
		'y', 'Y', 'Y', 'y',
		'u', 'U', 'U', 'u',
		'i', 'I', 'I', 'i',
		'o', 'O', 'O', 'o',
		'p', 'P', 'P', 'p',
		'[', '{', '[', '{',
		']', '}', ']', '}',
		'\n', '\n', '\n', '\n',
		0, 0, 0, 0, /* KBKEY_LCTRL */
		'a', 'A', 'A', 'a',
		's', 'S', 'S', 's',
		'd', 'D', 'D', 'd',
		'f', 'F', 'F', 'f',
		'g', 'G', 'G', 'g',
		'h', 'H', 'H', 'h',
		'j', 'J', 'J', 'j',
		'k', 'K', 'K', 'k',
		'l', 'L', 'L', 'l',
		';', ':', ';', ':',
		'\'', '"', '\'', '"',
		'`', '~', '`', '~',
		0, 0, 0, 0, /* KBKEY_LSHIFT */
		'\\', '|', '\\', '|',
		'z', 'Z', 'Z', 'z',
		'x', 'X', 'X', 'x',
		'c', 'C', 'C', 'c',
		'v', 'V', 'V', 'v',
		'b', 'B', 'B', 'b',
		'n', 'N', 'N', 'n',
		'm', 'M', 'M', 'm',
		',', '<', ',', '<',
		'.', '>', '.', '>',
		'/', '?', '/', '?',
		0, 0, 0, 0, /* KBKEY_RSHIFT */
		'*', '*', '*', '*',
		0, 0, 0, 0, /* KBKEY_LALT */
		' ', ' ', ' ', ' ',
		0, 0, 0, 0, /* KBKEY_CAPSLOCK */
		0, 0, 0, 0, /* KBKEY_F1 */
		0, 0, 0, 0, /* KBKEY_F2 */
		0, 0, 0, 0, /* KBKEY_F3 */
		0, 0, 0, 0, /* KBKEY_F4 */
		0, 0, 0, 0, /* KBKEY_F5 */
		0, 0, 0, 0, /* KBKEY_F6 */
		0, 0, 0, 0, /* KBKEY_F7 */
		0, 0, 0, 0, /* KBKEY_F8 */
		0, 0, 0, 0, /* KBKEY_F9 */
		0, 0, 0, 0, /* KBKEY_F10 */
		0, 0, 0, 0, /* KBKEY_NUMLOCK */
		0, 0, 0, 0, /* KBKEY_SCROLLLOCK */
		0, 0, 0, 0, /* KBKEY_KPAD7 */
		0, 0, 0, 0, /* KBKEY_KPAD8 */
		0, 0, 0, 0, /* KBKEY_KPAD9 */
		'-', '-', '-', '-',
		0, 0, 0, 0, /* KBKEY_KPAD4 */
		0, 0, 0, 0, /* KBKEY_KPAD5 */
		0, 0, 0, 0, /* KBKEY_KPAD6 */
		'+', '+', '+', '+',
		/* Nothing printable after this point */
	};

	KBLayoutUS::KBLayoutUS()
	{
		modifiers = 0;
	}

	KBLayoutUS::~KBLayoutUS()
	{
	}

	bool KBLayoutUS::ProcessModifier(int logickey, int modkey, unsigned flag)
	{
		if ( logickey == modkey ) { modifiers |= flag; return true; }
		if ( logickey == -modkey ) { modifiers &= ~flag; return true; }
		return false;
	}

	uint32_t KBLayoutUS::Translate(int kbkey)
	{
		if ( kbkey == KBKEY_LSHIFT ) { modifiers |= MOD_LSHIFT; return 0; }
		if ( kbkey == -KBKEY_LSHIFT ) { modifiers &= ~MOD_LSHIFT; return 0; }
		if ( kbkey == KBKEY_RSHIFT ) { modifiers |= MOD_RSHIFT; return 0; }
		if ( kbkey == -KBKEY_RSHIFT ) { modifiers &= ~MOD_RSHIFT; return 0; }
		if ( kbkey == KBKEY_CAPSLOCK ) { modifiers ^= MOD_CAPS; return 0; }

		int abskbkey = (kbkey < 0) ? -kbkey : kbkey;

		if ( (modifiers & MOD_LSHIFT) || (modifiers & MOD_RSHIFT) )
			modifiers |= MOD_SHIFT;
		else
			modifiers &= ~MOD_SHIFT;

		unsigned usedmods = modifiers & (MOD_SHIFT | MOD_CAPS);
		size_t index = (abskbkey<<2) | usedmods;

		// Check if the kbkey is outside the layout structure (not printable).
		size_t numchars = sizeof(LAYOUT_US) / 4UL / sizeof(uint32_t);
		if ( numchars < (size_t) abskbkey ) { return 0; }

		return LAYOUT_US[index];
	}
}

