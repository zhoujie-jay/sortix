/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2014.

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

    sortix/kblayout.h
    Binary data format describing the conversion of scancodes into codepoints.

*******************************************************************************/

#ifndef INCLUDE_SORTIX_KBLAYOUT_H
#define INCLUDE_SORTIX_KBLAYOUT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define KBLAYOUT_MAX_NUM_MODIFIERS 8

#define KBLAYOUT_COMPRESSION_NONE 0
#define KBLAYOUT_COMPRESSION_RUNLENGTH_BEVLQ 1

struct kblayout
{
	char magic[32]; // "sortix-kblayout-1"
	char name[32];
	uint32_t num_modifiers;
	uint32_t num_scancodes;
	uint32_t compression_algorithm;
};

#define KBLAYOUT_ACTION_TYPE_CODEPOINT 0
#define KBLAYOUT_ACTION_TYPE_DEAD 1
#define KBLAYOUT_ACTION_TYPE_MODIFY 2
#define KBLAYOUT_ACTION_TYPE_TOGGLE 3

struct kblayout_action
{
	uint32_t type;
	uint32_t codepoint;
};

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
