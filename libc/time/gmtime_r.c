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
 * time/gmtime_r.c
 * Convert a timestamp into a date and time according to UTC.
 */

#include <stdbool.h>
#include <time.h>

#include <stdint.h>
#include <time.h>

static const int DAYS_JANUARY = 31;
static const int DAYS_FEBRUARY = 28;
static const int DAYS_MARCH = 31;
static const int DAYS_APRIL = 30;
static const int DAYS_MAY = 31;
static const int DAYS_JUNE = 30;
static const int DAYS_JULY = 31;
static const int DAYS_AUGUST = 31;
static const int DAYS_SEPTEMBER = 30;
static const int DAYS_OCTOBER = 31;
static const int DAYS_NOVEMBER = 30;
static const int DAYS_DECEMBER = 31;

#define DECL_LEAP_SECOND(year, jun, dec) \
	{0, 0, 0, 0, 0, jun, 0, 0, 0, 0, 0, dec}

static int8_t leap_seconds[][12] =
{
	DECL_LEAP_SECOND(1970, 0, 0),
	DECL_LEAP_SECOND(1971, 0, 0),
	DECL_LEAP_SECOND(1972, 1, 1),
	DECL_LEAP_SECOND(1973, 0, 1),
	DECL_LEAP_SECOND(1974, 0, 1),
	DECL_LEAP_SECOND(1975, 0, 1),
	DECL_LEAP_SECOND(1976, 0, 1),
	DECL_LEAP_SECOND(1977, 0, 1),
	DECL_LEAP_SECOND(1978, 0, 1),
	DECL_LEAP_SECOND(1979, 0, 1),
	DECL_LEAP_SECOND(1980, 0, 0),
	DECL_LEAP_SECOND(1981, 1, 0),
	DECL_LEAP_SECOND(1982, 1, 0),
	DECL_LEAP_SECOND(1983, 1, 0),
	DECL_LEAP_SECOND(1984, 0, 0),
	DECL_LEAP_SECOND(1985, 1, 0),
	DECL_LEAP_SECOND(1986, 0, 0),
	DECL_LEAP_SECOND(1987, 0, 1),
	DECL_LEAP_SECOND(1988, 0, 0),
	DECL_LEAP_SECOND(1989, 0, 1),
	DECL_LEAP_SECOND(1990, 0, 1),
	DECL_LEAP_SECOND(1991, 0, 0),
	DECL_LEAP_SECOND(1992, 1, 0),
	DECL_LEAP_SECOND(1993, 1, 0),
	DECL_LEAP_SECOND(1994, 1, 0),
	DECL_LEAP_SECOND(1995, 0, 1),
	DECL_LEAP_SECOND(1996, 0, 0),
	DECL_LEAP_SECOND(1997, 1, 0),
	DECL_LEAP_SECOND(1998, 0, 1),
	DECL_LEAP_SECOND(1999, 0, 0),
	DECL_LEAP_SECOND(2000, 0, 0),
	DECL_LEAP_SECOND(2001, 0, 0),
	DECL_LEAP_SECOND(2002, 0, 0),
	DECL_LEAP_SECOND(2003, 0, 0),
	DECL_LEAP_SECOND(2004, 0, 0),
	DECL_LEAP_SECOND(2005, 0, 1),
	DECL_LEAP_SECOND(2006, 0, 0),
	DECL_LEAP_SECOND(2007, 0, 0),
	DECL_LEAP_SECOND(2008, 0, 1),
	DECL_LEAP_SECOND(2009, 0, 0),
	DECL_LEAP_SECOND(2010, 0, 0),
	DECL_LEAP_SECOND(2011, 0, 0),
	DECL_LEAP_SECOND(2012, 1, 0),
	DECL_LEAP_SECOND(2013, 0, 0),
	DECL_LEAP_SECOND(2014, 0, 0),
	DECL_LEAP_SECOND(2015, 1, 0),
};

static time_t get_leap_second(int year, int month)
{
	const time_t num_years = sizeof(leap_seconds) / sizeof(leap_seconds[0]);
	if ( year < 1970 )
		return 0;
	if ( num_years <= year-1970 )
		return 0;
	return leap_seconds[year-1970][month];
}

static time_t leap_seconds_in_year(int year)
{
	time_t ret = 0;
	for ( int i = 0; i < 12; i++ )
		ret += get_leap_second(year, i);
	return ret;
}

static bool is_leap_year(int year)
{
	return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

static time_t days_in_year(int year)
{
	return DAYS_JANUARY +
	       DAYS_FEBRUARY + (is_leap_year(year) ? 1 : 0) +
	       DAYS_MARCH +
	       DAYS_APRIL +
	       DAYS_MAY +
	       DAYS_JUNE +
	       DAYS_JULY +
	       DAYS_AUGUST +
	       DAYS_SEPTEMBER +
	       DAYS_OCTOBER +
	       DAYS_NOVEMBER +
	       DAYS_DECEMBER;
}

struct tm* gmtime_r(const time_t* time_ptr, struct tm* ret)
{
	time_t left = *time_ptr;

	ret->tm_year = 1970;
	ret->tm_wday = 4 /* Supposedly, the world began on a Thursday. */;

	// If the timestamp is after the epoch.
	while ( 0 < left )
	{
		time_t year_leaps = leap_seconds_in_year(ret->tm_year);
		time_t year_days = days_in_year(ret->tm_year);
		time_t year_seconds = year_days * 24 * 60 * 60 + year_leaps;
		if ( year_seconds <= left )
		{
			left -= year_seconds;
			ret->tm_wday = (ret->tm_wday + year_days) % 7;
			ret->tm_year++;
			continue;
		}
		break;
	}

	// If the timestamp was before the epoch.
	while ( left < 0 )
	{
		ret->tm_year--;
		time_t year_leaps = leap_seconds_in_year(ret->tm_year);
		time_t year_days = days_in_year(ret->tm_year);
		time_t year_seconds = year_days * 24 * 60 * 60 + year_leaps;
		left += year_seconds;
		// We need to avoid taking the modulo of a negative value or the
		// (broken) C modulo operator gives the wrong result.
		ret->tm_wday = (ret->tm_wday - year_days + 7*7*7*7) % 7;
	}

	int month_days_list[12] =
	{
		DAYS_JANUARY,
		DAYS_FEBRUARY + (is_leap_year(ret->tm_year) ? 1 : 0),
		DAYS_MARCH,
		DAYS_APRIL,
		DAYS_MAY,
		DAYS_JUNE,
		DAYS_JULY,
		DAYS_AUGUST,
		DAYS_SEPTEMBER,
		DAYS_OCTOBER,
		DAYS_NOVEMBER,
		DAYS_DECEMBER,
	};

	// Figure out the correct month.
	ret->tm_mon = 0;
	ret->tm_yday = 0;
	while ( true )
	{
		int month_leaps = get_leap_second(ret->tm_year, ret->tm_mon);
		int month_days = month_days_list[ret->tm_mon];
		int month_seconds = month_days * 24 * 60 * 60 + month_leaps;
		if ( month_seconds <= left )
		{
			left -= month_seconds;
			ret->tm_mon++;
			ret->tm_yday += month_days;
			ret->tm_wday = (ret->tm_wday + month_days) % 7;
			continue;
		}
		break;
	}

	ret->tm_mday = left / (24 * 60 * 60);
	left = left % (24 * 60 * 60);

	// If this is a regular timestamp.
	if ( ret->tm_mday < month_days_list[ret->tm_mon] )
	{
		ret->tm_yday += ret->tm_mday;

		ret->tm_hour = left / (60 * 60);
		left = left % (60 * 60);

		ret->tm_min = left / 60;
		left = left % 60;

		ret->tm_sec = left;
	}

	// If we got the timestamp for an added leap second.
	else
	{
		ret->tm_mday--; // Seemingly additional day.
		ret->tm_yday += ret->tm_mday;
		ret->tm_hour = 23;
		ret->tm_min = 59;
		ret->tm_sec = 60;
	}

	ret->tm_wday = (ret->tm_wday + ret->tm_mday) % 7;

	// TODO: Support daylight savings and timezones.
	ret->tm_isdst = -1;

	// Fix the ranges of some of the variables.
	ret->tm_mday += 1;
	ret->tm_year -= 1900;

	return ret;
}

time_t timegm(struct tm* tm)
{
	time_t year = tm->tm_year + 1900;
	time_t month = tm->tm_mon;
	time_t day = tm->tm_mday - 1;
	time_t hour = tm->tm_hour;
	time_t minute = tm->tm_min;
	time_t second = tm->tm_sec;

	time_t ret = 0;
	for ( time_t y = 1970; y < year; y++ )
	{
		time_t year_leaps = leap_seconds_in_year(y);
		time_t year_days = days_in_year(y);
		time_t year_seconds = year_days * 24 * 60 * 60 + year_leaps;
		ret += year_seconds;
	}

	int month_days_list[12] =
	{
		DAYS_JANUARY,
		DAYS_FEBRUARY + (is_leap_year(year) ? 1 : 0),
		DAYS_MARCH,
		DAYS_APRIL,
		DAYS_MAY,
		DAYS_JUNE,
		DAYS_JULY,
		DAYS_AUGUST,
		DAYS_SEPTEMBER,
		DAYS_OCTOBER,
		DAYS_NOVEMBER,
		DAYS_DECEMBER,
	};

	for ( uint8_t m = 0; m < month; m++ )
	{
		int month_leaps = get_leap_second(year, m);
		int month_days = month_days_list[m];
		int month_seconds = month_days * 24 * 60 * 60 + month_leaps;
		ret += month_seconds;
	}

	ret += (time_t) day * 24 * 60 * 60;
	ret += (time_t) hour * 60 * 60;
	ret += (time_t) minute * 60;
	ret += (time_t) second * 1;

	gmtime_r(&ret, tm);

	return ret;
}
