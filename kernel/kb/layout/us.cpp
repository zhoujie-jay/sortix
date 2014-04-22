/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2014.

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

    kb/layout/us.cpp
    The United States keyboard layout.

*******************************************************************************/

#include <stdint.h>

#include <sortix/keycodes.h>

#include <sortix/kernel/kernel.h>
#include <sortix/kernel/keyboard.h>

#include "us.h"

namespace Sortix {

const unsigned MOD_SHIFT = 1U << 0U;
const unsigned MOD_CAPS = 1U << 1U;
const unsigned MOD_LSHIFT = 1U << 2U;
const unsigned MOD_RSHIFT = 1U << 3U;

const uint32_t LAYOUT_US[4UL*128UL] =
{
	0, 0, 0, 0, /* unused: kbkey 0 is invalid */
	0, 0, 0, 0, /* KBKEY_ESC */
	L'1', L'!', L'1', L'!',
	L'2', L'@', L'2', L'@',
	L'3', L'#', L'3', L'#',
	L'4', L'$', L'4', L'$',
	L'5', L'%', L'5', L'%',
	L'6', L'^', L'6', L'^',
	L'7', L'&', L'7', L'&',
	L'8', L'*', L'8', L'*',
	L'9', L'(', L'9', L'(',
	L'0', L')', L'0', L')',
	L'-', L'_', L'-', L'_',
	L'=', L'+', L'=', L'+',
	L'\b', L'\b', L'\b', L'\b',
	L'\t', L'\t', L'\t', L'\t',
	L'q', L'Q', L'Q', L'q',
	L'w', L'W', L'W', L'w',
	L'e', L'E', L'E', L'e',
	L'r', L'R', L'R', L'r',
	L't', L'T', L'T', L't',
	L'y', L'Y', L'Y', L'y',
	L'u', L'U', L'U', L'u',
	L'i', L'I', L'I', L'i',
	L'o', L'O', L'O', L'o',
	L'p', L'P', L'P', L'p',
	L'[', L'{', L'[', L'{',
	L']', L'}', L']', L'}',
	L'\n', L'\n', L'\n', L'\n',
	0, 0, 0, 0, /* KBKEY_LCTRL */
	L'a', L'A', L'A', L'a',
	L's', L'S', L'S', L's',
	L'd', L'D', L'D', L'd',
	L'f', L'F', L'F', L'f',
	L'g', L'G', L'G', L'g',
	L'h', L'H', L'H', L'h',
	L'j', L'J', L'J', L'j',
	L'k', L'K', L'K', L'k',
	L'l', L'L', L'L', L'l',
	L';', L':', L';', L':',
	L'\'', L'"', L'\'', L'"',
	L'`', L'~', L'`', L'~',
	0, 0, 0, 0, /* KBKEY_LSHIFT */
	L'\\', L'|', L'\\', L'|',
	L'z', L'Z', L'Z', L'z',
	L'x', L'X', L'X', L'x',
	L'c', L'C', L'C', L'c',
	L'v', L'V', L'V', L'v',
	L'b', L'B', L'B', L'b',
	L'n', L'N', L'N', L'n',
	L'm', L'M', L'M', L'm',
	L',', L'<', L',', L'<',
	L'.', L'>', L'.', L'>',
	L'/', L'?', L'/', L'?',
	0, 0, 0, 0, /* KBKEY_RSHIFT */
	L'*', L'*', L'*', L'*',
	0, 0, 0, 0, /* KBKEY_LALT */
	L' ', L' ', L' ', L' ',
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
	L'-', L'-', L'-', L'-',
	0, 0, 0, 0, /* KBKEY_KPAD4 */
	0, 0, 0, 0, /* KBKEY_KPAD5 */
	0, 0, 0, 0, /* KBKEY_KPAD6 */
	L'+', L'+', L'+', L'+',
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
	if ( logickey == modkey )
		return modifiers |= flag, true;
	if ( logickey == -modkey )
		return modifiers &= ~flag, true;
	return false;
}

uint32_t KBLayoutUS::Translate(int kbkey)
{
	if ( kbkey == KBKEY_LSHIFT )
		return modifiers |= MOD_LSHIFT, 0;
	if ( kbkey == -KBKEY_LSHIFT )
		return modifiers &= ~MOD_LSHIFT, 0;
	if ( kbkey == KBKEY_RSHIFT )
		return modifiers |= MOD_RSHIFT, 0;
	if ( kbkey == -KBKEY_RSHIFT )
		return modifiers &= ~MOD_RSHIFT, 0;
	if ( kbkey == KBKEY_CAPSLOCK )
		return modifiers ^= MOD_CAPS, 0;

	int abskbkey = kbkey < 0 ? -kbkey : kbkey;

	if ( (modifiers & MOD_LSHIFT) || (modifiers & MOD_RSHIFT) )
		modifiers |= MOD_SHIFT;
	else
		modifiers &= ~MOD_SHIFT;

	unsigned usedmods = modifiers & (MOD_SHIFT | MOD_CAPS);
	size_t index = abskbkey << 2 | usedmods;

	// Check if the kbkey is outside the layout structure (not printable).
	size_t numchars = sizeof(LAYOUT_US) / 4UL / sizeof(uint32_t);
	if ( numchars < (size_t) abskbkey )
		return 0;

	return LAYOUT_US[index];
}

} // namespace Sortix
