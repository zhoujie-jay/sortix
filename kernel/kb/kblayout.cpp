/*
 * Copyright (c) 2014 Jonas 'Sortie' Termansen.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * kb/kblayout.cpp
 * Engine that executes a keyboard layout program.
 */

#include <assert.h>
#include <endian.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <sortix/kblayout.h>

#include <sortix/kernel/log.h>

#include "kblayout.h"

namespace Sortix {

KeyboardLayoutExecutor::KeyboardLayoutExecutor()
{
	memset(&header, 0, sizeof(header));
	memset(&modifier_counts, 0, sizeof(modifier_counts));
	loaded = false;
	actions = NULL;
	keys_down = NULL;
	modifiers = 0;
	saved_data = 0;
	saved_data_size = 0;
}

KeyboardLayoutExecutor::~KeyboardLayoutExecutor()
{
	delete[] actions;
	delete[] keys_down;
	delete[] saved_data;
}

bool KeyboardLayoutExecutor::Upload(const uint8_t* input_data, size_t input_size)
{
	size_t data_size = input_size;
	uint8_t* data = new uint8_t[data_size];
	if ( !data )
		return false;
	memcpy(data, input_data, data_size);
	struct kblayout new_header;
	if ( data_size < sizeof(struct kblayout) )
		return delete[] data, errno = EINVAL, false;
	memcpy(&new_header, data, sizeof(new_header));
	if ( strncmp(new_header.magic, "sortix-kblayout-1", sizeof(new_header.magic)) != 0 )
		return delete[] data, errno = EINVAL, false;
	new_header.num_modifiers = le32toh(new_header.num_modifiers);
	new_header.num_scancodes = le32toh(new_header.num_scancodes);
	if ( KBLAYOUT_MAX_NUM_MODIFIERS <= new_header.num_modifiers )
		return errno = EINVAL, false;
	// TODO: Limit num_scancodes and max_actions.
	size_t remaining_size = data_size - sizeof(new_header);
	// TODO: Check for overflow.
	size_t table_length = (size_t) (1 << new_header.num_modifiers) *
	                      (size_t) new_header.num_scancodes;
	size_t table_size = sizeof(struct kblayout_action) * table_length;
	if ( remaining_size < table_size )
		return delete[] data, errno = EINVAL, false;
	struct kblayout_action* new_actions = new struct kblayout_action[table_length];
	if ( !new_actions )
		return delete[] data, false;
	uint8_t* new_keys_down = new uint8_t[new_header.num_scancodes];
	if ( !new_keys_down )
		return delete[] data, delete[] new_actions, false;
	memcpy(new_actions, data + sizeof(new_header), table_size);
	for ( size_t i = 0; i < table_length; i++ )
		new_actions[i] = le32toh(new_actions[i]);
	memcpy(&header, &new_header, sizeof(header));
	delete[] actions;
	actions = new_actions;
	memset(new_keys_down, 0, sizeof(keys_down[0]) * new_header.num_scancodes);
	delete[] keys_down;
	keys_down = new_keys_down;
	memset(&modifier_counts, 0, sizeof(modifier_counts));
	modifiers = 0;
	delete[] saved_data;
	saved_data = data;
	saved_data_size = data_size;
	loaded = true;
	return true;
}

bool KeyboardLayoutExecutor::Download(const uint8_t** data, size_t* data_size)
{
	if ( !saved_data )
		return errno = EINIT, false;
	return *data = saved_data, *data_size = saved_data_size, true;
}

uint32_t KeyboardLayoutExecutor::Translate(int kbkey)
{
	if ( !loaded )
		return 0;
	unsigned int abskbkey = kbkey < 0 ? - (unsigned int) kbkey : kbkey;
	if ( header.num_scancodes <= abskbkey )
		return 0;

	bool repeated_key = 0 <= kbkey && keys_down[abskbkey];
	keys_down[abskbkey] = 0 <= kbkey ? 1 : 0;

	size_t action_index = abskbkey * (1 << header.num_modifiers) + modifiers;
	struct kblayout_action* action = &actions[action_index];
	if ( 0 < kbkey && action->type == KBLAYOUT_ACTION_TYPE_CODEPOINT )
		return action->codepoint;
	// TODO: Properly implement dead keys!
	if ( 0 < kbkey && action->type == KBLAYOUT_ACTION_TYPE_DEAD )
		return action->codepoint;
	if ( repeated_key )
		return 0;
	if ( action->type == KBLAYOUT_ACTION_TYPE_MODIFY &&
	     action->codepoint < header.num_modifiers )
	{
		if ( kbkey < 0 )
		{
			if ( --modifier_counts[action->codepoint] == 0 )
				modifiers &= ~(1 << action->codepoint);
		}
		else
		{
			modifier_counts[action->codepoint]++;
			modifiers |= 1 << action->codepoint;
		}
		return 0;
	}
	if ( 0 <= kbkey &&
	     action->type == KBLAYOUT_ACTION_TYPE_TOGGLE &&
	     action->codepoint < header.num_modifiers )
	{
		if ( (modifier_counts[action->codepoint] = !modifier_counts[action->codepoint]) )
			modifiers |= 1 << action->codepoint;
		else
			modifiers &= ~(1 << action->codepoint);
		return 0;
	}
	return 0;
}

} // namespace Sortix
