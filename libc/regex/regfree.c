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

    regex/regfree.c
    Regular expression freeing.

*******************************************************************************/

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
