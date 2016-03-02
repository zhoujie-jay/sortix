/*
 * Copyright (c) 2014, 2015 Jonas 'Sortie' Termansen.
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
 * regex/regerror.c
 * Regular expression error reporting.
 */

#include <regex.h>
#include <stdio.h>
#include <string.h>

size_t regerror(int errnum,
                const regex_t* restrict regex,
                char* restrict errbuf,
                size_t errbuf_size)
{
	(void) regex;
	const char* msg = "Unknown regular expression error";
	switch ( errnum )
	{
	case REG_NOMATCH: msg = "Regular expression does not match"; break;
	case REG_BADPAT: msg = "Invalid regular expression"; break;
	case REG_ECOLLATE: msg = "Invalid collating element referenced"; break;
	case REG_ECTYPE: msg = "Invalid character class type referenced"; break;
	case REG_EESCAPE: msg = "Trailing <backslash> character in pattern"; break;
	case REG_ESUBREG: msg = "Number in \\digit invalid or in error"; break;
	case REG_EBRACK: msg = "\"[]\" imbalance"; break;
	case REG_EPAREN: msg = "\"\\(\\)\" or \"()\" imbalance"; break;
	case REG_EBRACE: msg = "\"\\{\\}\" imbalance"; break;
	case REG_BADBR: msg = "Content of \"\\{\\}\" invalid: not a number, number too large, more than two numbers, first larger than second"; break;
	case REG_ERANGE: msg = "Invalid endpoint in range expression"; break;
	case REG_ESPACE: msg = "Out of memory"; break;
	case REG_BADRPT: msg = "'?', '*', or '+' not preceded by valid regular expression"; break;
	}
	if ( errbuf_size )
		strlcpy(errbuf, msg, errbuf_size);
	return strlen(msg) + 1;
}
