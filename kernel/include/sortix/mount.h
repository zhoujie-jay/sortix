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
 * sortix/mount.h
 * Constants related to mounting and binding.
 */

#ifndef SORTIX_INCLUDE_MOUNT_H
#define SORTIX_INCLUDE_MOUNT_H

#include <sys/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UNMOUNT_FORCE (1 << 0)
#define UNMOUNT_DETACH (1 << 1)
#define UNMOUNT_NOFOLLOW (1 << 2)

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
