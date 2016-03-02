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
 * sortix/kblayout.h
 * Binary data format describing the conversion of scancodes into codepoints.
 */

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
