/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

    This file is part of Sortix.

    Sortix is free software: you can redistribute it and/or modify it under the
    terms of the GNU General Public License as published by the Free Software
    Foundation, either version 3 of the License, or (at your option) any later
    version.

    Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
    details.

    You should have received a copy of the GNU General Public License along with
    Sortix. If not, see <http://www.gnu.org/licenses/>.

    x86-family/cmos.h
    Provides access to CMOS and the real-time clock.

*******************************************************************************/

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
