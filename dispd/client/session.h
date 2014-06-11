/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012.

    This file is part of dispd.

    dispd is free software: you can redistribute it and/or modify it under the
    terms of the GNU Lesser General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    dispd is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
    details.

    You should have received a copy of the GNU Lesser General Public License
    along with dispd. If not, see <http://www.gnu.org/licenses/>.

    session.h
    Handles session management.

*******************************************************************************/

#ifndef INCLUDE_DISPD_SESSION_H
#define INCLUDE_DISPD_SESSION_H

#if defined(__cplusplus)
extern "C" {
#endif

struct dispd_session
{
	size_t refcount;
	uint64_t device;
	uint64_t connector;
	struct dispd_window* current_window;
	bool is_rgba;
};

bool dispd__session_initialize(int* argc, char*** argv);

#if defined(__cplusplus)
} // extern "C"
#endif

#endif
