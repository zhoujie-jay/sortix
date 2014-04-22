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

    kb/kblayout.h
    Engine that executes a keyboard layout program.

*******************************************************************************/

#ifndef SORTIX_KB_KBLAYOUT_H
#define SORTIX_KB_KBLAYOUT_H

#include <stddef.h>
#include <stdint.h>

#include <sortix/kblayout.h>

namespace Sortix {

class KeyboardLayoutExecutor
{
public:
	KeyboardLayoutExecutor();
	~KeyboardLayoutExecutor();

public:
	bool Upload(const uint8_t* data, size_t data_size);
	bool Download(const uint8_t** data, size_t* data_size);
	uint32_t Translate(int kbkey);

private:
	struct kblayout header;
	struct kblayout_action* actions;
	uint8_t* keys_down;
	uint32_t modifier_counts[KBLAYOUT_MAX_NUM_MODIFIERS];
	uint32_t modifiers;
	bool loaded;
	uint8_t* saved_data;
	size_t saved_data_size;

};

} // namespace Sortix

#endif
