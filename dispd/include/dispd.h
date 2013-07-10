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

    dispd.h
    Interface to the Sortix Display Daemon.

*******************************************************************************/

#ifndef INCLUDE_DISPD_H
#define INCLUDE_DISPD_H

#if !defined(__cplusplus)
#include <stdbool.h>
#endif

#if defined(__cplusplus)
extern "C" {
#endif

struct dispd_session;
struct dispd_window;
struct dispd_framebuffer;

bool dispd_initialize(int* argc, char*** argv);

struct dispd_session* dispd_attach_default_session();
bool dispd_detach_session(struct dispd_session* session);

bool dispd_session_setup_game_vga(struct dispd_session* session);
bool dispd_session_setup_game_rgba(struct dispd_session* session);

struct dispd_window* dispd_create_window_game_vga(struct dispd_session* session);
struct dispd_window* dispd_create_window_game_rgba(struct dispd_session* session);
bool dispd_destroy_window(struct dispd_window* window);

struct dispd_framebuffer* dispd_begin_render(struct dispd_window* window);
bool dispd_finish_render(struct dispd_framebuffer* framebuffer);

uint8_t* dispd_get_framebuffer_data(struct dispd_framebuffer* framebuffer);
size_t dispd_get_framebuffer_pitch(struct dispd_framebuffer* framebuffer);
int dispd_get_framebuffer_format(struct dispd_framebuffer* framebuffer);
int dispd_get_framebuffer_height(struct dispd_framebuffer* framebuffer);
int dispd_get_framebuffer_width(struct dispd_framebuffer* framebuffer);

#if defined(__cplusplus)
} // extern "C"
#endif

#endif
