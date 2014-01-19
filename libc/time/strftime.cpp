/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

    This file is part of the Sortix C Library.

    The Sortix C Library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    The Sortix C Library is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
    License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with the Sortix C Library. If not, see <http://www.gnu.org/licenses/>.

    time/strftime.cpp
    Format time and date into a string.

*******************************************************************************/

#include <errno.h>
#include <stdio.h>
#include <time.h>

static const char* GetWeekdayAbbreviated(const struct tm* tm)
{
	switch ( tm->tm_wday % 7 )
	{
	case 0: return "Sun";
	case 1: return "Mon";
	case 2: return "Tue";
	case 3: return "Wed";
	case 4: return "Thu";
	case 5: return "Fri";
	case 6: return "Sat";
	default: __builtin_unreachable();
	}
}

static const char* GetWeekday(const struct tm* tm)
{
	switch ( tm->tm_wday % 7 )
	{
	case 0: return "Sunday";
	case 1: return "Monday";
	case 2: return "Tuesday";
	case 3: return "Wednesday";
	case 4: return "Thursday";
	case 5: return "Friday";
	case 6: return "Saturday";
	default: __builtin_unreachable();
	}
}

static const char* GetMonthAbbreviated(const struct tm* tm)
{
	switch ( tm->tm_mon % 12 )
	{
	case 0: return "Jan";
	case 1: return "Feb";
	case 2: return "Mar";
	case 3: return "Apr";
	case 4: return "May";
	case 5: return "Jun";
	case 6: return "Jul";
	case 7: return "Aug";
	case 8: return "Sep";
	case 9: return "Oct";
	case 10: return "Nov";
	case 11: return "Dec";
	default: __builtin_unreachable();
	}
}

static const char* GetMonth(const struct tm* tm)
{
	switch ( tm->tm_mon % 12 )
	{
	case 0: return "January";
	case 1: return "February";
	case 2: return "March";
	case 3: return "April";
	case 4: return "May";
	case 5: return "June";
	case 6: return "July";
	case 7: return "August";
	case 8: return "Sepember";
	case 9: return "October";
	case 10: return "November";
	case 11: return "December";
	default: __builtin_unreachable();
	}
}

extern "C"
size_t strftime(char* s, size_t max, const char* format, const struct tm* tm)
{
	const char* orig_format = format;
	size_t ret = 0;

#define OUTPUT_CHAR(expr) \
	do { \
		if ( ret == max ) \
			return errno = ERANGE, 0; \
		s[ret++] = expr; \
	} while ( 0 )

#define OUTPUT_STRING(expr) \
	do { \
		const char* out_str = expr; \
		while ( *out_str ) \
			OUTPUT_CHAR(*out_str++); \
	} while ( 0 )

#define OUTPUT_STRFTIME(expr) \
	do { \
		int old_errno = errno; \
		errno = 0; \
		size_t subret = strftime(s + ret, max - ret, expr, tm); \
		if ( !subret && errno ) \
			return 0; \
		errno = old_errno; \
		ret += subret; \
	} while ( 0 )

#define OUTPUT_INT_PADDED(valexpr, widthexpr, padexpr) \
	do { \
		int val = valexpr; \
		size_t width = widthexpr; \
		char pad = padexpr; \
		char str[sizeof(int) * 3]; str[0] = '\0'; \
		size_t got = (size_t) snprintf(str, sizeof(str), "%i", val); \
		while ( pad && got < width-- ) \
			OUTPUT_CHAR(pad); \
		OUTPUT_STRING(str); \
	} while ( 0 )

#define OUTPUT_INT(valexpr) OUTPUT_INT_PADDED(valexpr, 0,'\0')

#define OUTPUT_UNSUPPORTED() \
	do { \
		fprintf(stderr, "%s:%u: strftime: error: unsupported format string \"%s\" around \"%%%s\"\n", \
		        __FILE__, __LINE__, orig_format, specifiers_begun_at); \
		return 0; \
	} while ( 0 )

#define OUTPUT_UNDEFINED() OUTPUT_UNSUPPORTED()

	while ( char c = *format++ )
	{
		if ( c != '%' )
		{
			OUTPUT_CHAR(c);
			continue;
		}
		const char* specifiers_begun_at = format;
		c = *format++;

		// Process any optional flags.
		char padding_char = ' ';
		bool plus_padding = false;
		while ( true )
		{
			switch ( c )
			{
			case '0': padding_char = '0'; break;
			case '+': padding_char = '0'; plus_padding = true; break;
			default: goto end_of_flags;
			}
			c = *format++;
		}
	end_of_flags:

		// TODO: Support the '+' flag!
		if ( plus_padding )
			OUTPUT_UNSUPPORTED();

		// Process the optional minimum field width.
		size_t field_width = 0;
		while ( '0' <= c && c <= '9' )
			field_width = field_width * 10 + c - '0',
			c = *format++;

		// Process an optional E or O modifier.
		bool e_modifier = false;
		bool o_modifier = false;
		if ( c == 'E' )
			e_modifier = true,
			c = *format++;
		else if ( c == 'O' )
			o_modifier = true,
			c = *format++;

		// TODO: Support the E and O modifiers where marked with the E and O
		//       comments in the below switch.
		if ( e_modifier || o_modifier ) { }

		switch ( c )
		{
		case 'a': OUTPUT_STRING(GetWeekdayAbbreviated(tm)); break;
		case 'A': OUTPUT_STRING(GetWeekday(tm)); break;
		case 'b': OUTPUT_STRING(GetMonthAbbreviated(tm)); break;
		case 'B': OUTPUT_STRING(GetMonth(tm)); break;
		case 'c': /*E*/
			OUTPUT_STRING(GetWeekday(tm));
			OUTPUT_STRING(" ");
			OUTPUT_STRING(GetMonthAbbreviated(tm));
			OUTPUT_STRING(" ");
			OUTPUT_INT_PADDED(tm->tm_mday, 2, '0');
			OUTPUT_STRING(" ");
			OUTPUT_INT_PADDED(tm->tm_hour, 2, '0');
			OUTPUT_STRING(":");
			OUTPUT_INT_PADDED(tm->tm_min, 2, '0');
			OUTPUT_STRING(":");
			OUTPUT_INT_PADDED(tm->tm_sec, 2, '0');
			break;
		case 'C': /*E*/
			if ( !field_width )
				field_width = 2;
			OUTPUT_INT_PADDED((tm->tm_year + 1900) / 100, field_width, padding_char);
			break;
		case 'd': OUTPUT_INT_PADDED(tm->tm_mday, 2, '0'); break; /*O*/
		case 'D': OUTPUT_STRFTIME("%m/%d/%y"); break;
		case 'e': OUTPUT_INT_PADDED(tm->tm_mday, 2, ' '); break; /*O*/
		case 'F':
			// TODO: Revisit this.
			OUTPUT_UNSUPPORTED();
			break;
		case 'g':
			// TODO: These require a bit of intelligence.
			OUTPUT_UNSUPPORTED();
			break;
		case 'G':
			// TODO: These require a bit of intelligence.
			OUTPUT_UNSUPPORTED();
			break;
		case 'h': OUTPUT_STRFTIME("%b"); break;
		case 'H': OUTPUT_INT_PADDED(tm->tm_hour, 2, '0'); break; /*O*/
		case 'I': OUTPUT_INT_PADDED(tm->tm_hour % 12 + 1, 2, '0'); break; /*O*/
		case 'j': OUTPUT_INT_PADDED(tm->tm_yday + 1, 3, '0'); break;
		case 'm': OUTPUT_INT_PADDED(tm->tm_mon + 1, 2, '0'); break; /*O*/
		case 'M': OUTPUT_INT_PADDED(tm->tm_min, 2, '0'); break; /*O*/
		case 'n': OUTPUT_CHAR('\n'); break;
		case 'p': OUTPUT_STRING(tm->tm_hour < 12 ? "AM" : "PM"); break;
		case 'r': OUTPUT_STRFTIME("%I:%M:%S %p"); break;
		case 'R': OUTPUT_STRFTIME("%H:%M"); break;
		case 'S': OUTPUT_INT_PADDED(tm->tm_sec, 2, '0'); break; /*O*/
		case 't': OUTPUT_CHAR('\t'); break;
		case 'T': OUTPUT_STRFTIME("%H:%M:%S"); break;
		case 'u': OUTPUT_INT(tm->tm_yday); break; /*O*/
		case 'U': /*O*/
			// TODO: These require a bit of intelligence.
			OUTPUT_UNSUPPORTED();
			break;
		case 'V': /*O*/
			// TODO: These require a bit of intelligence.
			OUTPUT_UNSUPPORTED();
			break;
		case 'w': OUTPUT_INT(tm->tm_wday); break; /*O*/
		case 'W': /*O*/
			// TODO: These require a bit of intelligence.
			OUTPUT_UNSUPPORTED();
			break;
		case 'x': OUTPUT_STRFTIME("%m/%d/%y"); break; /*E*/
		case 'X': OUTPUT_STRFTIME("%H:%M:%S"); break; /*E*/
		case 'y': OUTPUT_INT_PADDED((tm->tm_year + 1900) % 100, 2, '0'); break; /*EO*/
		case 'Y': OUTPUT_INT(tm->tm_year + 1900); break; /*E*/
		case 'z':
			// TODO: struct tm doesn't have all this information available!
			break;
		case 'Z':
			// TODO: struct tm doesn't have all this information available!
			break;
		case '%': OUTPUT_CHAR('%'); break;
		default: OUTPUT_UNDEFINED(); break;
		}
	}
	if ( ret == max )
		return errno = ERANGE, 0;
	return s[ret] = '\0', ret;
}
