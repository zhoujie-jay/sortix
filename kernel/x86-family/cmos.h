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
 * x86-family/cmos.h
 * Provides access to CMOS and the real-time clock.
 */

#ifndef SORTIX_X86_FAMILY_CMOS_H
#define SORTIX_X86_FAMILY_CMOS_H

#include <sortix/timespec.h>

namespace Sortix {
namespace CMOS {

uint8_t Read(uint8_t reg);
uint8_t Write(uint8_t reg, uint8_t val);

struct cmos_tm
{
	uint8_t seconds;
	uint8_t minutes;
	uint8_t hours;
	uint8_t weekday; /* Supposedly not reliably set in CMOS. */
	uint8_t day_of_month;
	uint8_t month;
	uint8_t year;
	uint8_t century;
};

struct timespec DecodeTime(struct cmos_tm time);
struct cmos_tm EncodeTIme(struct timespec time);

struct cmos_tm GetTime();
void SetTime(struct cmos_tm time);

void Init();

} // namespace CMOS
} // namespace Sortix

#endif
