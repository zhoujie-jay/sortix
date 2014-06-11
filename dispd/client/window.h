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

    window.h
    Handles windows.

*******************************************************************************/

#ifndef INCLUDE_DISPD_WINDOW_H
#define INCLUDE_DISPD_WINDOW_H

#if defined(__cplusplus)
extern "C" {
#endif

struct dispd_window
{
	struct dispd_session* session;
	uint8_t* cached_buffer;
	size_t cached_buffer_size;
};

#if defined(__cplusplus)
} // extern "C"
#endif

#endif
