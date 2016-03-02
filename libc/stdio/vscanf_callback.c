/*
 * Copyright (c) 2012, 2014, 2016 Jonas 'Sortie' Termansen.
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
 * stdio/vscanf_callback.c
 * Input format conversion.
 */

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

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

int vscanf_callback(void* fp,
                    int (*fgetc)(void*),
                    int (*ungetc)(int, void*),
                    const char* restrict format,
                    va_list ap)
{
	int matcheditems = 0;
	size_t fieldwidth = 0;
	bool escaped = false;
	bool discard = false;
	bool negint = false;
	bool intunsigned = false;
	bool leadingzero = false;
	bool hasprefix = false;
	bool string = false;
	size_t intparsed = 0;
	uintmax_t intvalue = 0;
	int ic;
	int base = 0;
	int cval;
	const size_t UNDO_MAX = 4;
	int undodata[UNDO_MAX];
	size_t undoable = 0;
	size_t strwritten = 0;
	char* strdest = NULL;
	char convc;
	enum scantype scantype = TYPE_INT;
	enum scanmode scanmode = MODE_INIT;
	while ( true )
	{
		ic = fgetc(fp);
		unsigned char uc = ic; char c = uc;
		switch (scanmode)
		{
		case MODE_INIT:
			if ( !*format )
			{
				ungetc(ic, fp);
				goto break_loop;
			}
			if ( isspace((unsigned char) *format) )
			{
				if ( isspace(ic) )
					continue;
				else
					do format++;
					while ( isspace((unsigned char) *format) );
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

			switch ( (convc = *format++) )
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
			if ( !intparsed && c == '-' && !intunsigned && !negint )
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
