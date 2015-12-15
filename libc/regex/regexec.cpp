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

    regex/regexec.cpp
    Regular expression execution.

*******************************************************************************/

#include <assert.h>
#include <regex.h>
#include <pthread.h>

#define QUEUE_CURRENT_STATE(new_state) \
{ \
	if ( !new_state ) \
	{ \
		match = true; \
		for ( struct re* re = state->re_current_state_next; \
		      re; \
		      re = re->re_current_state_next ) \
			re->re_is_current = 0; \
		state->re_current_state_next = NULL; \
		current_states_last = state; \
	} \
	else if ( !(new_state->re_is_current && new_state->re_is_currently_done) ) \
	{ \
		if ( new_state->re_is_current ) \
		{ \
			if ( new_state->re_current_state_prev ) \
				new_state->re_current_state_prev->re_current_state_next = \
					new_state->re_current_state_next; \
			else \
				current_states = new_state->re_current_state_next; \
			if ( new_state->re_current_state_next ) \
				new_state->re_current_state_next->re_current_state_prev = \
					new_state->re_current_state_prev; \
			else \
				current_states_last = new_state->re_current_state_prev; \
		} \
		new_state->re_current_state_prev = state; \
		new_state->re_current_state_next = state->re_current_state_next; \
		if ( state->re_current_state_next ) \
			state->re_current_state_next->re_current_state_prev = new_state; \
		else \
			current_states_last = new_state; \
		state->re_current_state_next = new_state; \
		new_state->re_is_currently_done = 0; \
		new_state->re_is_current = 1; \
		new_state->re_is_upcoming = 0; \
		for ( size_t m = 0; m < nmatch; m++ ) \
			new_state->re_matches[m] = state->re_matches[m]; \
	} \
} \

#define QUEUE_UPCOMING_STATE(new_state) \
{ \
	if ( !new_state ) \
	{ \
		consumed_char = true; \
		match = true; \
		for ( struct re* re = state->re_current_state_next; \
		      re; \
		      re = re->re_current_state_next ) \
			re->re_is_current = 0; \
		state->re_current_state_next = NULL; \
		current_states_last = state; \
	} \
	else if ( !new_state->re_is_upcoming ) \
	{ \
		if ( !upcoming_states ) \
			upcoming_states = new_state; \
		if ( upcoming_states_last ) \
			upcoming_states_last->re_upcoming_state_next = new_state; \
		upcoming_states_last = new_state; \
		new_state->re_upcoming_state_next = NULL; \
		new_state->re_is_upcoming = 1; \
		for ( size_t m = 0; m < nmatch; m++ ) \
			new_state->re_matches[m] = state->re_matches[m]; \
	} \
} \

extern "C"
int regexec(const regex_t* restrict regex_const,
            const char* restrict string,
            size_t nmatch,
            regmatch_t* restrict pmatch,
            int eflags)
{
	// TODO: Sanitize eflags.

	regex_t* regex = (regex_t*) regex_const;
	pthread_mutex_lock(&regex->re_lock);

	if ( regex->re_cflags & REG_NOSUB )
		nmatch = 0;

	for ( size_t i = 0; i < nmatch; i++ )
	{
		pmatch[i].rm_so = -1;
		pmatch[i].rm_eo = -1;
	}

	if ( regex->re_nsub + 1 < nmatch )
		nmatch = regex->re_nsub + 1;

	int result = REG_NOMATCH;

	struct re* current_states = NULL;
	struct re* current_states_last = NULL;
	struct re* upcoming_states = NULL;
	struct re* upcoming_states_last = NULL;

	regex->re->re_is_current = 0;

	for ( size_t i = 0; true; i++ )
	{
		if ( !regex->re->re_is_current && result == REG_NOMATCH )
		{
			if ( current_states_last )
				current_states_last->re_current_state_next = regex->re;
			else
				current_states = regex->re;
			regex->re->re_current_state_prev = current_states_last;
			regex->re->re_current_state_next = NULL;
			current_states_last = regex->re;
			regex->re->re_is_currently_done = 0;
			regex->re->re_is_current = 1;
			regex->re->re_is_upcoming = 0;
			for ( size_t m = 0; m < nmatch; m++ )
			{
				regex->re->re_matches[m].rm_so = m == 0 ? i : -1;
				regex->re->re_matches[m].rm_eo = -1;
			}
		}
		char c = string[i];
		for ( struct re* state = current_states;
		      state;
		      state = state->re_current_state_next )
		{
			bool match = false;
			bool consumed_char = false;
			if ( state->re_type == RE_TYPE_BOL )
			{
				if ( !(eflags & REG_NOTBOL) )
					QUEUE_CURRENT_STATE(state->re_next);
			}
			else if ( state->re_type == RE_TYPE_EOL )
			{
				if ( !(eflags & REG_NOTEOL) && c == '\0' )
					QUEUE_CURRENT_STATE(state->re_next);
			}
			else if ( state->re_type == RE_TYPE_CHAR )
			{
				if ( c != '\0' && state->re_char.c == c )
					QUEUE_UPCOMING_STATE(state->re_next);
			}
			else if ( state->re_type == RE_TYPE_ANY_CHAR )
			{
				if ( c != '\0' )
					QUEUE_UPCOMING_STATE(state->re_next);
			}
			else if ( state->re_type == RE_TYPE_SET )
			{
				unsigned char uc = c;
				if ( c != '\0' && (state->re_set.set[uc / 8] & (1 << (uc % 8))) )
					QUEUE_UPCOMING_STATE(state->re_next);
			}
			else if ( state->re_type == RE_TYPE_SUBEXPRESSION )
			{
				size_t index = state->re_subexpression.index;
				state->re_matches[index].rm_so = i;
				QUEUE_CURRENT_STATE(state->re_next);
			}
			else if ( state->re_type == RE_TYPE_SUBEXPRESSION_END )
			{
				size_t index = state->re_subexpression.index;
				state->re_matches[index].rm_eo = i;
				QUEUE_CURRENT_STATE(state->re_next);
			}
			else if ( state->re_type == RE_TYPE_ALTERNATIVE ||
			          state->re_type == RE_TYPE_OPTIONAL ||
			          state->re_type == RE_TYPE_LOOP )
			{
				QUEUE_CURRENT_STATE(state->re_split.re);
				QUEUE_CURRENT_STATE(state->re_next);
			}
			state->re_is_currently_done = 1;
			if ( match )
			{
				state->re_matches[0].rm_eo = i + consumed_char;
				for ( size_t m = 0; m < nmatch; m++ )
					pmatch[m] = state->re_matches[m];
				result = 0;
				if ( nmatch == 0 )
					break;
			}
		}

		for ( struct re* re = current_states; re; re = re->re_current_state_next )
			re->re_is_current = 0;

		if ( nmatch == 0 && result == 0 )
		{
			for ( struct re* re = upcoming_states; re; re = re->re_upcoming_state_next )
				re->re_is_upcoming = 0;
			break;
		}

		current_states = upcoming_states;
		if ( current_states )
			current_states->re_current_state_prev = NULL;
		current_states_last = upcoming_states_last;
		for ( struct re* re = current_states; re; re = re->re_current_state_next )
		{
			re->re_is_currently_done = 0;
			re->re_is_current = 1;
			re->re_is_upcoming = 0;
			re->re_current_state_next = re->re_upcoming_state_next;
			if ( re->re_current_state_next )
				re->re_current_state_next->re_current_state_prev = re;
		}
		upcoming_states = NULL;
		upcoming_states_last = NULL;

		eflags |= REG_NOTBOL;

		if ( current_states == NULL && result == 0 )
			break;

		if ( c == '\0' )
			break;
	}

	pthread_mutex_unlock(&regex->re_lock);

	return result;
}
