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

	localeconv.cpp
	Return locale-specific information.

*******************************************************************************/

#include <locale.h>

static struct lconv lc;

extern "C" struct lconv* localeconv(void)
{
	lc.decimal_point = (char*) ".";
	lc.thousands_sep = (char*) "";
	lc.grouping = (char*) "";
	lc.int_curr_symbol = (char*) "";
	lc.currency_symbol = (char*) "";
	lc.mon_decimal_point = (char*) "";
	lc.mon_thousands_sep = (char*) "";
	lc.mon_grouping = (char*) "";
	lc.positive_sign = (char*) "";
	lc.negative_sign = (char*) "";
	lc.int_frac_digits = 127;
	lc.frac_digits = 127;
	lc.p_cs_precedes = 127;
	lc.n_cs_precedes = 127;
	lc.p_sep_by_space = 127;
	lc.n_sep_by_space = 127;
	lc.p_sign_posn = 127;
	lc.n_sign_posn = 127;
	lc.int_p_cs_precedes = 127;
	lc.int_n_cs_precedes = 127;
	lc.int_p_sep_by_space = 127;
	lc.int_n_sep_by_space = 127;
	lc.int_p_sign_posn = 127;
	lc.int_n_sign_posn = 127;
	return &lc;
}