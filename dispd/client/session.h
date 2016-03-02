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
 * session.h
 * Handles session management.
 */

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
} /* extern "C" */
#endif

#endif
