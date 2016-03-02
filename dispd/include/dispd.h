/*
 * Copyright (c) 2012 Jonas 'Sortie' Termansen.
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
 * dispd.h
 * Interface to the Sortix Display Daemon.
 */

#ifndef INCLUDE_DISPD_H
#define INCLUDE_DISPD_H

#if !defined(__cplusplus)
#include <stdbool.h>
#endif
#include <stddef.h>
#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct dispd_session;
struct dispd_window;
struct dispd_framebuffer;

bool dispd_initialize(int* argc, char*** argv);

struct dispd_session* dispd_attach_default_session(void);
bool dispd_detach_session(struct dispd_session* session);

bool dispd_session_setup_game_rgba(struct dispd_session* session);

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
} /* extern "C" */
#endif

#endif
