/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2014, 2015.

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

    regex/regerror.cpp
    Regular expression error reporting.

*******************************************************************************/

#include <regex.h>
#include <stdio.h>
#include <string.h>

extern "C"
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
