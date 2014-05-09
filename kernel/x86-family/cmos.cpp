/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2014.

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

    x86-family/cmos.cpp
    Provides access to CMOS and the real-time clock.

*******************************************************************************/

#include <sys/types.h>

#include <stdint.h>
#include <time.h>
#include <timespec.h>

#include <sortix/clock.h>

#include <sortix/kernel/clock.h>
#include <sortix/kernel/cpu.h>
#include <sortix/kernel/ioport.h>
#include <sortix/kernel/kernel.h>
#include <sortix/kernel/time.h>

namespace Sortix {
namespace CMOS {

const uint16_t CMOS_ADDRESS_REG = 0x70;
const uint16_t CMOS_DATA_REG = 0x71;

uint8_t ReadRTC(uint8_t reg)
{
	outport8(CMOS_ADDRESS_REG, reg);
	return inport8(CMOS_DATA_REG);
}

bool IsRTCUpdateInProgress()
{
	return ReadRTC(0x0A) & 0x80;
}

uint8_t DecodeBCD(uint8_t bcd)
{
	return bcd / 16 * 10 + bcd % 16;
}

struct rtc_values
{
	uint8_t second;
	uint8_t minute;
	uint8_t hour;
	uint8_t day;
	uint8_t month;
	uint8_t year;
	uint8_t century;
	uint8_t reg_b;
};

bool are_rtc_values_equal(struct rtc_values a, struct rtc_values b)
{
	return a.second == b.second &&
	       a.minute == b.minute &&
	       a.hour == b.hour &&
	       a.day == b.day &&
	       a.month == b.month &&
	       a.year == b.year &&
	       a.century == b.century &&
	       a.reg_b == b.reg_b;
}

struct rtc_values ReadRTCValues()
{
	while ( IsRTCUpdateInProgress() );
	struct rtc_values result;
	result.second = ReadRTC(0x00);
	result.minute = ReadRTC(0x02);
	result.hour = ReadRTC(0x04);
	result.day = ReadRTC(0x07);
	result.month = ReadRTC(0x08);
	result.year = ReadRTC(0x09);
	result.century = ReadRTC(0x32);
	result.reg_b = ReadRTC(0x0B);
	return result;
}

void Init()
{
	struct rtc_values values_first = ReadRTCValues();
	struct rtc_values values_second = ReadRTCValues();
	while ( !are_rtc_values_equal(values_first, values_second) )
	{
		values_first = values_second;
		values_second = ReadRTCValues();
	}

	uint8_t second = values_first.second;
	uint8_t minute = values_first.minute;
	uint8_t hour = values_first.hour;
	uint8_t day = values_first.day;
	uint8_t month = values_first.month;
	uint8_t year = values_first.year;
	uint8_t century = values_first.century;
	uint8_t reg_b = values_first.reg_b;

	bool hour12 = !(reg_b & 0x02);
	bool is_pm = hour12 && hour & 0x80;
	if ( hour12 )
		hour &= 0x7F;

	if ( !(reg_b & 0x04) )
	{
		second = DecodeBCD(second);
		minute = DecodeBCD(minute);
		hour = DecodeBCD(hour);
		day = DecodeBCD(day);
		month = DecodeBCD(month);
		year = DecodeBCD(year);
		century = DecodeBCD(century);
	}

	if ( hour12 && is_pm )
		hour = (hour + 12) % 24;

	time_t full_year = century * 100 + year;

	struct tm tm;
	memset(&tm, 0, sizeof(tm));
	tm.tm_sec = second;
	tm.tm_min = minute;
	tm.tm_hour = hour;
	tm.tm_mday = day ;
	tm.tm_mon = month - 1;
	tm.tm_year = full_year - 1900;
	time_t now = timegm(&tm);

	struct timespec current_time = timespec_make(now, 0);
	Time::GetClock(CLOCK_REALTIME)->Set(&current_time, NULL);
}

} // namespace CMOS
} // namespace Sortix
