/*
 * Copyright (c) 2016 Jonas 'Sortie' Termansen.
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
 * sortix/ioctl.h
 * Miscellaneous device control interface.
 */

#ifndef INCLUDE_SORTIX_IOCTL_H
#define INCLUDE_SORTIX_IOCTL_H

#define __IOCTL_TYPE_EXP 3 /* 2^3 kinds of argument types supported.*/
#define __IOCTL_TYPE_MASK ((1 << __IOCTL_TYPE_EXP) - 1)
#define __IOCTL_TYPE_VOID 0
#define __IOCTL_TYPE_INT 1
#define __IOCTL_TYPE_LONG 2
#define __IOCTL_TYPE_PTR 3
/* 4-7 is unused in case of future expansion. */
#define __IOCTL(index, type) ((index) << __IOCTL_TYPE_EXP | (type))
#define __IOCTL_INDEX(value) ((value) >> __IOCTL_TYPE_EXP)
#define __IOCTL_TYPE(value) ((value) & __IOCTL_TYPE_MASK)

#define TIOCGWINSZ __IOCTL(1, __IOCTL_TYPE_PTR)

#endif
