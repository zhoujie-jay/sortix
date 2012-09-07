/*******************************************************************************

	Copyright(C) Jonas 'Sortie' Termansen 2012.

	This file is part of LibMaxsi.

	LibMaxsi is free software: you can redistribute it and/or modify it under
	the terms of the GNU Lesser General Public License as published by the Free
	Software Foundation, either version 3 of the License, or (at your option)
	any later version.

	LibMaxsi is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
	details.

	You should have received a copy of the GNU Lesser General Public License
	along with LibMaxsi. If not, see <http://www.gnu.org/licenses/>.

	vfscanf.cpp
	Input format conversion.

*******************************************************************************/

#define __STDC_LIMIT_MACROS
#include <ctype.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>

enum scanmode
{
	MODE_INIT,
	MODE_CONVSPEC,
	MODE_SCANINT,
	MODE_SCANINT_REAL,
	MODE_SCANSTRING,
	MODE_SCANSTRING_REAL,
};

enum scantype
{
	TYPE_SHORT,
	TYPE_SHORTSHORT,
	TYPE_INT,
	TYPE_LONG,
	TYPE_LONGLONG,
	TYPE_SIZE,
	TYPE_PTRDIFF,
	TYPE_MAX,
};

static bool IsTypeModifier(char c)
{
	return c == 'h' || c == 'j' || c == 'l' || c == 'L' || c == 't' || c == 'z';
}

static int debase(char c, int base)
{
	if ( c == '0' )
		return 0;
	int ret = -1;
	if ( '0' <= c && c <= '9' ) { ret = c - '0' +  0; }
	if ( 'a' <= c && c <= 'f' ) { ret = c - 'a' + 10; }
	if ( 'A' <= c && c <= 'F' ) { ret = c - 'A' + 10; }
	if ( base <= ret )
		return -1;
	return ret;
}

extern "C" int vfscanf(FILE* fp, const char* origformat, va_list ap)
{
	union { const char* format; const unsigned char* formatuc; };
	format = origformat;
	int matcheditems = 0;
	size_t fieldwidth;
	bool escaped = false;
	bool discard;
	bool negint;
	bool intunsigned;
	bool leadingzero;
	bool hasprefix;
	bool string;
	size_t intparsed;
	uintmax_t intvalue;
	int ic;
	int base;
	int cval;
	const size_t UNDO_MAX = 4;
	int undodata[UNDO_MAX];
	size_t undoable = 0;
	size_t strwritten;
	char* strdest;
	enum scantype scantype;
	enum scanmode scanmode = MODE_INIT;
	while ( true )
	{
		ic = fgetc(fp);
		unsigned char uc = ic; char c = uc;
		switch (scanmode)
		{
		case MODE_INIT:
			if ( !*format )
				goto break_loop;
			if ( isspace(*formatuc) )
			{
				if ( isspace(ic) )
					continue;
				else
					do format++;
					while ( isspace(*formatuc) );
			}
			if ( *format == '%' && !escaped )
			{
				format++;
				scanmode = MODE_CONVSPEC;
				ungetc(ic, fp);
				continue;
			}
			escaped = false;
			if ( *format != c ) { ungetc(ic, fp); goto break_loop; }
			format++;
			break;
		case MODE_CONVSPEC:
			discard = false;
			if ( *format == '*' ) { discard = true; format++; }
			fieldwidth = 0;
			while ( '0'<= *format && *format <= '9' )
				fieldwidth = fieldwidth * 10 + *format++ - '0';
			scantype = TYPE_INT;
			while ( IsTypeModifier(*format) )
				switch ( *format++ )
				{
				case 'h': scantype = scantype == TYPE_SHORT ? TYPE_SHORTSHORT
				                                            : TYPE_SHORT; break;
				case 'j': scantype = TYPE_MAX; break;
				case 'l': scantype = scantype == TYPE_LONG ? TYPE_LONGLONG
				                                           : TYPE_LONG; break;
				case 'L': scantype = TYPE_LONGLONG; break;
				case 't': scantype = TYPE_PTRDIFF; break;
				case 'z': scantype = TYPE_SIZE; break;
				}

			switch ( char convc = *format++ )
			{
			case '%':
				escaped = true;
			default:
				fprintf(stderr, "Warning: scanf does not support %c (%i)\n",
				        convc, convc);
				fprintf(stderr, "Bailing out to prevent problems.\n");
				errno = ENOTSUP;
				return -1;
				continue;
			case 'd':
				base = 10; scanmode = MODE_SCANINT; intunsigned = false; break;
			case 'i':
				base =  0; scanmode = MODE_SCANINT; intunsigned = false; break;
			case 'o':
				base =  0; scanmode = MODE_SCANINT; intunsigned = true; break;
			case 'u':
				base = 10; scanmode = MODE_SCANINT; intunsigned = true; break;
			case 'x':
			case 'X':
				base = 16; scanmode = MODE_SCANINT; intunsigned = true; break;
			case 'c':
				string = false; scanmode = MODE_SCANSTRING; break;
			case 's':
				string = true; scanmode = MODE_SCANSTRING; break;
			}
			ungetc(ic, fp);
			continue;
		case MODE_SCANINT:
			intparsed = 0;
			intvalue = 0;
			leadingzero = false;
			negint = false;
			hasprefix = false;
			undoable = 0;
			scanmode = MODE_SCANINT_REAL;
		case MODE_SCANINT_REAL:
			if ( fieldwidth )
			{
				fprintf(stderr, "Error: field width not supported for integers in scanf.\n");
				errno = ENOTSUP;
				return -1;
			}
			if ( !undoable && isspace(ic) )
				continue;
			if ( undoable < UNDO_MAX )
				undodata[undoable++] = ic;
			if ( c == '-' && !intunsigned && !negint )
			{
				negint = true;
				continue;
			}
			if ( !intparsed && c == '0' && !hasprefix &&
			     (!base || base == 8 || base == 16) && !leadingzero )
				leadingzero = true;
			if ( intparsed == 1 && (c == 'x' || c == 'X') && !hasprefix &&
			     (!base || base == 16) && leadingzero )
			{
				base = 16;
				leadingzero = false;
				hasprefix = true;
				intparsed = 0;
				continue;
			}
			else if ( intparsed == 1 && '1' <= c && c <= '7' && !hasprefix &&
			          (!base || base == 8) && leadingzero )
			{
				base = 8;
				hasprefix = true;
				leadingzero = false;
			}
			else if ( !intparsed && '0' <= c && c <= '9' && !hasprefix &&
			          (!base || base == 10) && !leadingzero )
			{
				base = 10;
				leadingzero = false;
				hasprefix = true;
			}
			cval = debase(c, base);
			if ( cval < 0 )
			{
				if ( !intparsed )
				{
					while ( undoable )
						ungetc(undodata[--undoable], fp);
					goto break_loop;
				}
				scanmode = MODE_INIT;
				undoable = 0;
				ungetc(ic, fp);
				if ( discard ) { discard = false; continue; }
				uintmax_t uintmaxval = intvalue;
				// TODO: Possible truncation of INTMAX_MIN!
				intmax_t intmaxval = uintmaxval;
				if ( negint ) intmaxval = -intmaxval;
				bool un = intunsigned;
				switch ( scantype )
				{
				case TYPE_SHORTSHORT:
					if ( un ) *va_arg(ap, unsigned char*) = uintmaxval;
					else *va_arg(ap, signed char*) = intmaxval;
					break;
				case TYPE_SHORT:
					if ( un ) *va_arg(ap, unsigned short*) = uintmaxval;
					else *va_arg(ap, signed short*) = intmaxval;
					break;
				case TYPE_INT:
					if ( un ) *va_arg(ap, unsigned int*) = uintmaxval;
					else *va_arg(ap, signed int*) = intmaxval;
					break;
				case TYPE_LONG:
					if ( un ) *va_arg(ap, unsigned long*) = uintmaxval;
					else *va_arg(ap, signed long*) = intmaxval;
					break;
				case TYPE_LONGLONG:
					if ( un ) *va_arg(ap, unsigned long long*) = uintmaxval;
					else *va_arg(ap, signed long long*) = intmaxval;
					break;
				case TYPE_PTRDIFF:
					*va_arg(ap, ptrdiff_t*) = intmaxval;
					break;
				case TYPE_SIZE:
					if ( un ) *va_arg(ap, size_t*) = uintmaxval;
					else *va_arg(ap, ssize_t*) = intmaxval;
					break;
				case TYPE_MAX:
					if ( un ) *va_arg(ap, uintmax_t*) = uintmaxval;
					else *va_arg(ap, intmax_t*) = intmaxval;
					break;
				}
				matcheditems++;
				continue;
			}
			intvalue = intvalue * (uintmax_t) base + (uintmax_t) cval;
			intparsed++;
			continue;
		case MODE_SCANSTRING:
			if ( !fieldwidth )
				fieldwidth = string ? SIZE_MAX : 1;
			scanmode = MODE_SCANSTRING_REAL;
			strwritten = 0;
			strdest = discard ? NULL : va_arg(ap, char*);
		case MODE_SCANSTRING_REAL:
			if ( string && !strwritten && isspace(ic) )
				continue;
			if ( string && strwritten &&
			     (ic == EOF || isspace(ic) || strwritten == fieldwidth) )
			{
				ungetc(ic, fp);
				if ( !discard )
					strdest[strwritten] = '\0';
				matcheditems++;
				scanmode = MODE_INIT;
				continue;
			}
			if ( !string && strwritten == fieldwidth )
			{
				ungetc(ic, fp);
				scanmode = MODE_INIT;
				continue;
			}
			if ( ic == EOF )
				goto break_loop;
			if ( !discard )
				strdest[strwritten++] = c;
			continue;
		}
	}
break_loop:
	return matcheditems;
}
