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
 * regex/regfree.c
 * Regular expression freeing.
 */

#include <regex.h>
#include <stdlib.h>

void regfree(regex_t* regex)
{
	struct re* parent = NULL;
	struct re* re = regex->re;
	while ( re )
	{
		if ( re->re_type == RE_TYPE_SUBEXPRESSION && re->re_subexpression.re_owner )
		{
			re->re_next = parent;
			parent = re;
			re = parent->re_subexpression.re_owner;
			parent->re_subexpression.re_owner = NULL;
			continue;
		}
		if ( (re->re_type == RE_TYPE_ALTERNATIVE ||
		      re->re_type == RE_TYPE_OPTIONAL ||
		      re->re_type == RE_TYPE_LOOP) &&
		     re->re_split.re_owner )
		{
			re->re_next = parent;
			parent = re;
			re = parent->re_split.re_owner;
			parent->re_split.re_owner = NULL;
			continue;
		}
		if ( re->re_type == RE_TYPE_REPETITION && re->re_repetition.re )
		{
			re->re_next = parent;
			parent = re;
			re = parent->re_repetition.re;
			parent->re_repetition.re = NULL;
			continue;
		}
		struct re* todelete = re;
		re = re->re_next_owner;
		if ( !re && parent )
		{
			re = parent;
			parent = re->re_next;
		}
		free(todelete);
	}
	free(regex->re_matches);
	pthread_mutex_destroy(&regex->re_lock);
}
