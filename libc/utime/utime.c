/*
 * Copyright (c) 2013 Jonas 'Sortie' Termansen.
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
 * utime/utime.c
 * Change file last access and modification times.
 */

#include <sys/stat.h>
#include <sys/types.h>

#include <utime.h>
#include <time.h>
#include <timespec.h>

// TODO: This interface has been deprecated by utimens (although that's a
//       non-standard Sortix extension, in which case by utimensat) - it'd be
//       nice to remove this at some point.
int utime(const char* filepath, const struct utimbuf* times)
{
	struct timespec ts_times[2];
	ts_times[0] = timespec_make(times->actime, 0);
	ts_times[1] = timespec_make(times->modtime, 0);
	return utimens(filepath, ts_times);
}
