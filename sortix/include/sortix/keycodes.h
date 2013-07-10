/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012.

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

    sortix/keycodes.h
    Defines codes for every logical key on keyboards.

*******************************************************************************/

#ifndef SORTIX_KEYCODES_H
#define SORTIX_KEYCODES_H

/* Each of these keycodes corrospond to a logical key on a keyboard. Code
   reading logical keystrokes from the keyboard will receive a code from this
   list when the key is pressed, and a negative version of the same key when it
   is released by the user. */

#define KBKEY_ESC 0x01
#define KBKEY_NUM1 0x02
#define KBKEY_NUM2 0x03
#define KBKEY_NUM3 0x04
#define KBKEY_NUM4 0x05
#define KBKEY_NUM5 0x06
#define KBKEY_NUM6 0x07
#define KBKEY_NUM7 0x08
#define KBKEY_NUM8 0x09
#define KBKEY_NUM9 0x0A
#define KBKEY_NUM0 0x0B
#define KBKEY_SYM1 0x0C
#define KBKEY_SYM2 0x0D
#define KBKEY_BKSPC 0x0E
#define KBKEY_TAB 0x0F
#define KBKEY_Q 0x10
#define KBKEY_W 0x11
#define KBKEY_E 0x12
#define KBKEY_R 0x13
#define KBKEY_T 0x14
#define KBKEY_Y 0x15
#define KBKEY_U 0x16
#define KBKEY_I 0x17
#define KBKEY_O 0x18
#define KBKEY_P 0x19
#define KBKEY_SYM3 0x1A
#define KBKEY_SYM4 0x1B
#define KBKEY_ENTER 0x1C
#define KBKEY_LCTRL 0x1D
#define KBKEY_A 0x1E
#define KBKEY_S 0x1F
#define KBKEY_D 0x20
#define KBKEY_F 0x21
#define KBKEY_G 0x22
#define KBKEY_H 0x23
#define KBKEY_J 0x24
#define KBKEY_K 0x25
#define KBKEY_L 0x26
#define KBKEY_SYM5 0x27
#define KBKEY_SYM6 0x28
#define KBKEY_SYM7 0x29
#define KBKEY_LSHIFT 0x2A
#define KBKEY_SYM8 0x2B
#define KBKEY_Z 0x2C
#define KBKEY_X 0x2D
#define KBKEY_C 0x2E
#define KBKEY_V 0x2F
#define KBKEY_B 0x30
#define KBKEY_N 0x31
#define KBKEY_M 0x32
#define KBKEY_SYM9 0x33
#define KBKEY_SYM10 0x34
#define KBKEY_SYM11 0x35
#define KBKEY_RSHIFT 0x36
#define KBKEY_SYM12 0x37
#define KBKEY_LALT 0x38
#define KBKEY_SPACE 0x39
#define KBKEY_CAPSLOCK 0x3A
#define KBKEY_F1 0x3B
#define KBKEY_F2 0x3C
#define KBKEY_F3 0x3D
#define KBKEY_F4 0x3E
#define KBKEY_F5 0x3F
#define KBKEY_F6 0x40
#define KBKEY_F7 0x41
#define KBKEY_F8 0x42
#define KBKEY_F9 0x43
#define KBKEY_F10 0x44
#define KBKEY_NUMLOCK 0x45
#define KBKEY_SCROLLLOCK 0x46
#define KBKEY_KPAD7 0x47
#define KBKEY_KPAD8 0x48
#define KBKEY_KPAD9 0x49
#define KBKEY_SYM13 0x4A
#define KBKEY_KPAD4 0x4B
#define KBKEY_KPAD5 0x4C
#define KBKEY_KPAD6 0x4D
#define KBKEY_SYM14 0x4E
#define KBKEY_KPAD1 0x4F
#define KBKEY_KPAD2 0x50
#define KBKEY_KPAD3 0x51
#define KBKEY_KPAD0 0x52
#define KBKEY_SYM15 0x53
#define KBKEY_ALTSYSRQ 0x54
#define KBKEY_NO_STANDARD_MEANING_1 0x55 /* Sometimes F11, F12, or even FN */
#define KBKEY_NO_STANDARD_MEANING_2 0x56 /* Possibly Windows key? */
#define KBKEY_F11 0x57
#define KBKEY_F12 0x58
/* [0x59, 0x7F] are not really standard. */
#define KBKEY_KPADENTER (0x80 + 0x1C)
#define KBKEY_RCTRL (0x80 + 0x1D)
#define KBKEY_FAKELSHIFT (0x80 + 0x2A)
#define KBKEY_SYM16 (0x80 + 0x35)
#define KBKEY_FAKERSHIFT (0x80 + 0x36)
#define KBKEY_CTRLPRINTSCRN (0x80 + 0x37)
#define KBKEY_RALT (0x80 + 0x38)
#define KBKEY_CTRLBREAK (0x80 + 0x46)
#define KBKEY_HOME (0x80 + 0x47)
#define KBKEY_UP (0x80 + 0x48)
#define KBKEY_PGUP (0x80 + 0x49)
#define KBKEY_LEFT (0x80 + 0x4B)
#define KBKEY_RIGHT (0x80 + 0x4D)
#define KBKEY_END (0x80 + 0x4F)
#define KBKEY_DOWN (0x80 + 0x50)
#define KBKEY_PGDOWN (0x80 + 0x51)
#define KBKEY_INSERT (0x80 + 0x52)
#define KBKEY_DELETE (0x80 + 0x53)
#define KBKEY_LSUPER (0x80 + 0x5B)
#define KBKEY_RSUPER (0x80 + 0x5C)
#define KBKEY_MENU (0x80 + 0x5D)

#define KBKEY_ENCODE(_kbkey) \
({ \
	int __kbkey = (_kbkey); \
	uint32_t __codepoint = (1U<<30U) | ((unsigned) __kbkey); \
	__codepoint &= ~(1U<<31U); \
	__codepoint; \
})

#define KBKEY_DECODE(_codepoint) \
({ \
	struct { union { int __kbkey; uint32_t __codepoint; }; } __u; \
	__u.__codepoint = (_codepoint); \
	if ( __u.__codepoint & (1U<<30U) ) \
	{ \
		if ( __u.__codepoint & (1U<<29U) ) { __u.__codepoint |= (1U<<31U); } \
		else { __u.__codepoint &= ~(1U<<30U); } \
	} \
	else  \
	{ \
		__u.__codepoint = 0U; \
	} \
	__u.__kbkey; \
})

#endif
