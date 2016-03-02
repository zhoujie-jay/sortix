/*
 * Copyright (c) 2012 Jonas 'Sortie' Termansen.
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
 * locale/localeconv.c
 * Return locale-specific information.
 */

#include <locale.h>

static struct lconv lc;

struct lconv* localeconv(void)
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
