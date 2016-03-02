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
 * regex/regcomp.c
 * Regular expression compiler.
 */

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <regex.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct re_parse_subexpr
{
	struct re_parse_subexpr* next;
	struct re** prev_next_ptr;
	struct re** primary_next_ptr;
};

struct re_parse
{
	struct re_parse_subexpr* subexpr;
	size_t subexpr_num;
};

static inline bool re_basic_well_defined_escape(char c)
{
	return c == '\\' || c == '(' || c == ')' || c == '{' || c == '}' ||
	       c == '.' || c == '*' || c == '[' || c == ']' || c == '^' ||
	       c == '$' || c == '+' || c == '?' || c == '|' ||
	       ('0' <= c && c <= '9');
}

static inline bool re_extended_well_defined_escape(char c)
{
	return c == '\\' || c == '(' || c == ')' || c == '{' || c == '}' ||
	       c == '.' || c == '*' || c == '[' || c == ']' || c == '^' ||
	       c == '$' || c == '+' || c == '?' || c == '|';
}

static inline void re_free(struct re* re)
{
	regex_t regex;
	memset(&regex, 0, sizeof(regex));
	pthread_mutex_init(&regex.re_lock, NULL);
	regex.re = re;
	regfree(&regex);
}

static inline int re_parse(struct re_parse* parse,
                           struct re** restrict prev_next_ptr,
                           const char* restrict pattern,
                           int cflags)
{
	*prev_next_ptr = NULL;

	bool is_extended = cflags & REG_EXTENDED;
	bool is_basic = !is_extended;

	struct re** primary_next_ptr = prev_next_ptr;
	struct re* re;

	size_t pattern_index = 0;
	//size_t alternative_begun_at = pattern_index;
	while ( true )
	{
		size_t c_pattern_index = pattern_index++;
		char c = pattern[c_pattern_index];

		if ( c == '\0' )
		{
			if ( parse->subexpr )
				return REG_EPAREN;
			return 0;
		}

		bool escaped = false;
		if ( c == '\\' )
		{
			c_pattern_index = pattern_index++;
			c = pattern[c_pattern_index];
			if ( c == '\0' )
				return REG_BADPAT;
			if ( is_basic && !re_basic_well_defined_escape(c) )
				return REG_BADPAT;
			if ( is_extended && !re_extended_well_defined_escape(c) )
				return REG_BADPAT;
			escaped = true;
		}

		bool escaped_for_basic = (is_basic && escaped) ||
		                         (is_extended && !escaped);

		if ( escaped_for_basic && c == ')' )
		{
			struct re_parse_subexpr* subexpr = parse->subexpr;
			if ( !subexpr )
				return REG_EPAREN;
			*prev_next_ptr = NULL;
			prev_next_ptr = subexpr->prev_next_ptr;
			primary_next_ptr = subexpr->primary_next_ptr;
			//alternative_begun_at = subexpr->alternative_begun_at;
			parse->subexpr = subexpr->next;
			free(subexpr);
			re = *prev_next_ptr;
			goto subexpression_done;
		}

		// TODO: Properly reject anchors in the basic regular expression cases
		//       where they aren't appropriate. Mind that we implement the
		//       extension where all ERE features are available in BRE mode if
		//       accessed through backslashes.
		//if ( !escaped && c == '^' &&
		//     (0 < parse->subexpr_depth || c_pattern_index != alternative_begun_at) )
		//	return REG_BADRPT;
		//if ( !escaped && c == '$' &&
		//     (0 < parse->subexpr_depth || pattern[pattern_index] != '0') )
		//	return REG_BADRPT;
		if ( !escaped && c == '*' )
			return REG_BADRPT;
		if ( escaped_for_basic && c == '{' )
			return REG_BADBR;
		if ( (is_basic && escaped && c == '+') ||
		     (is_extended && !escaped && c == '+') )
			return REG_BADBR;
		if ( (is_basic && escaped && c == '?') ||
		     (is_extended && !escaped && c == '?') )
			return REG_BADBR;

		if ( !(re = (struct re*) calloc(1, sizeof(struct re))) )
			return REG_ESPACE;

		if ( escaped_for_basic && c == '|' )
		{
			re->re_type = RE_TYPE_ALTERNATIVE;
			re->re_next_owner = *primary_next_ptr;
			re->re_split.re_owner = NULL;
			*primary_next_ptr = re;
			prev_next_ptr = primary_next_ptr = &re->re_split.re_owner;
			continue;
		}
		// TODO: Check if this anchor logic is the right one. This uses them as
		//       special characters in BRE mode in cases they shouldn't be.
		else if ( !escaped && c == '^' )
		{
			re->re_type = RE_TYPE_BOL;
			*prev_next_ptr = re;
			prev_next_ptr = &re->re_next_owner;
			continue;
		}
		else if ( !escaped && c == '$' )
		{
			re->re_type = RE_TYPE_EOL;
			*prev_next_ptr = re;
			prev_next_ptr = &re->re_next_owner;
			continue;
		}
		else if ( escaped_for_basic && c == '(' )
		{
			re->re_type = RE_TYPE_SUBEXPRESSION;
			re->re_subexpression.index = parse->subexpr_num++;
			re->re_subexpression.re_owner = NULL;
			*prev_next_ptr = re;
			struct re* end = (struct re*) calloc(1, sizeof(struct re));
			if ( !end )
				return REG_ESPACE;
			end->re_type = RE_TYPE_SUBEXPRESSION_END;
			end->re_subexpression.index = re->re_subexpression.index;
			re->re_next_owner = end;
			struct re_parse_subexpr* subexpr = (struct re_parse_subexpr*)
				calloc(sizeof(struct re_parse_subexpr), 1);
			if ( !subexpr )
				return REG_ESPACE;
			subexpr->prev_next_ptr = prev_next_ptr;
			subexpr->primary_next_ptr = primary_next_ptr;
			//subexpr->alternative_begun_at = alternative_begun_at;
			subexpr->next = parse->subexpr;
			parse->subexpr = subexpr;
			prev_next_ptr = &re->re_subexpression.re_owner;
			primary_next_ptr = &re->re_subexpression.re_owner;
			//alternative_begun_at = pattern_index;
			continue;
		}
		// TODO: This is not properly implemented.
		// TODO: This is not properly unicode-aware.
		else if ( c == '[' )
		{
			re->re_type = RE_TYPE_SET;
			bool negate = false;
			if ( pattern[pattern_index] == '^' )
			{
				pattern_index += 1;
				negate = true;
			}
			while ( pattern[pattern_index] != ']' )
			{
				if ( pattern[pattern_index] == '\0' )
					return free(re), REG_EBRACK;
				// TODO: This is wrong and fragile.
				unsigned char c_from;
				unsigned char c_to;
				if ( pattern[pattern_index + 1] == '-' )
				{
					c_from = (unsigned char) pattern[pattern_index + 0];
					c_to = (unsigned char) pattern[pattern_index + 2];
					pattern_index += 3;
				}
				else
				{
					c_from = (unsigned char) pattern[pattern_index + 0];
					c_to = (unsigned char) pattern[pattern_index + 0];
					pattern_index += 1;
				}
				for ( unsigned int uc = c_from; uc <= c_to; uc++ )
				{
					size_t byte_index = uc / 8;
					size_t bit_index = uc % 8;
					re->re_set.set[byte_index] |= (1 << bit_index);
				}
			}
			if ( negate )
			{
				for ( size_t i = 0; i < 32; i++ )
					re->re_set.set[i] = ~re->re_set.set[i];
			}
			if ( pattern[pattern_index++] != ']' )
				return free(re), REG_EBRACK;
		}
		else if ( escaped && ('0' <= c && c <= '9') )
		{
			// TODO: This isn't implemented yet (not part of ERE).
			return free(re), REG_BADPAT;
		}
		else if ( !escaped && c == '.' )
			re->re_type = RE_TYPE_ANY_CHAR;
		else
		{
			re->re_type = RE_TYPE_CHAR;
			re->re_char.c = c;
		}

		*prev_next_ptr = re;

subexpression_done:
		if ( (is_basic && pattern[pattern_index + 0] == '\\' &&
		                  pattern[pattern_index + 1] == '{') ||
		     (is_extended && pattern[pattern_index] == '{' ) )
		{
			pattern_index += is_extended ? 1 : 2;
			if ( pattern[pattern_index] < '0' ||
			     pattern[pattern_index] > '9' )
				return REG_BADBR;
			uintmax_t repeat_min;
			uintmax_t repeat_max;
			const char* value;
			const char* value_end;
			int saved_errno = errno;
			value = (char*) (pattern + pattern_index);
			repeat_min = strtoumax((char*) value, (char**) &value_end, 10);
			int parse_errno = errno;
			errno = saved_errno;
			if ( parse_errno == ERANGE || SIZE_MAX < repeat_min )
				return REG_BADBR;
			pattern_index += value_end - value;
			if ( pattern[pattern_index] == ',' )
			{
				repeat_max = SIZE_MAX;
				pattern_index += 1;
				if ( pattern[pattern_index] >= '0' &&
				     pattern[pattern_index] <= '9' )
				{
					saved_errno = errno;
					value = (char*) (pattern + pattern_index);
					repeat_max = strtoumax((char*) value, (char**) &value_end, 10);
					parse_errno = errno;
					errno = saved_errno;
					if ( parse_errno == ERANGE || SIZE_MAX < repeat_max )
						return  REG_BADBR;
					if ( repeat_max < repeat_min )
						return REG_BADBR;
					pattern_index += value_end - value;
				}
			}
			else
			{
				repeat_max = repeat_min;
			}
			if ( (is_basic && pattern[pattern_index++] != '\\') ||
			     pattern[pattern_index++] != '}' )
				return REG_BADBR;
			struct re* re_repetition = (struct re*) calloc(1, sizeof(struct re));
			if ( !re_repetition )
				return REG_ESPACE;
			re_repetition->re_type = RE_TYPE_REPETITION;
			re_repetition->re_repetition.re = re;
			re_repetition->re_repetition.min = (size_t) repeat_min;
			re_repetition->re_repetition.max = (size_t) repeat_max;
			*prev_next_ptr = re_repetition;
			re = re_repetition;
		}
		else if ( pattern[pattern_index] == '*' )
		{
			pattern_index += 1;
			struct re* re_repetition = (struct re*) calloc(1, sizeof(struct re));
			if ( !re_repetition )
				return REG_ESPACE;
			re_repetition->re_type = RE_TYPE_REPETITION;
			re_repetition->re_repetition.re = re;
			re_repetition->re_repetition.min = 0;
			re_repetition->re_repetition.max = SIZE_MAX;
			*prev_next_ptr = re_repetition;
			re = re_repetition;
		}
		else if ( (is_basic && pattern[pattern_index + 0] == '\\' &&
		                       pattern[pattern_index + 1] == '?') ||
		          (is_extended && pattern[pattern_index] == '?' ) )
		{
			pattern_index += is_extended ? 1 : 2;
			struct re* re_repetition = (struct re*) calloc(1, sizeof(struct re));
			if ( !re_repetition )
				return REG_ESPACE;
			re_repetition->re_type = RE_TYPE_REPETITION;
			re_repetition->re_repetition.re = re;
			re_repetition->re_repetition.min = 0;
			re_repetition->re_repetition.max = 1;
			*prev_next_ptr = re_repetition;
			re = re_repetition;
		}
		else if ( (is_basic && pattern[pattern_index + 0] == '\\' &&
		                       pattern[pattern_index + 1] == '+') ||
		          (is_extended && pattern[pattern_index] == '+' ) )
		{
			pattern_index += is_extended ? 1 : 2;
			struct re* re_repetition = (struct re*) calloc(1, sizeof(struct re));
			if ( !re_repetition )
				return REG_ESPACE;
			re_repetition->re_type = RE_TYPE_REPETITION;
			re_repetition->re_repetition.re = re;
			re_repetition->re_repetition.min = 1;
			re_repetition->re_repetition.max = SIZE_MAX;
			*prev_next_ptr = re_repetition;
			re = re_repetition;
		}

		if ( re->re_type == RE_TYPE_SUBEXPRESSION )
			re = re->re_next_owner; // RE_TYPE_SUBEXPRESSION_END.

		prev_next_ptr = &re->re_next_owner;
	}
}

static inline bool re_duplicate(struct re* templ, struct re** re_ptr)
{
	struct re* copy;
	struct re* parent_templ = NULL;
	struct re* parent_copy = NULL;
	while ( true )
	{
		if ( !templ )
		{
			if ( parent_templ )
			{
				templ = parent_templ;
				copy = parent_copy;
				parent_templ = templ->re_upcoming_state_next;
				parent_copy = copy->re_upcoming_state_next;
				templ = templ->re_next_owner;
				re_ptr = &copy->re_next_owner;
				continue;
			}
			return *re_ptr = NULL, true;
		}
		if ( !(copy = (struct re*) calloc(1, sizeof(struct re))) )
			return false;
		*re_ptr = copy;
		copy->re_type = templ->re_type;
		if ( templ->re_type == RE_TYPE_BOL )
			;
		else if ( templ->re_type == RE_TYPE_BOL )
			;
		else if ( templ->re_type == RE_TYPE_CHAR )
			copy->re_char.c = templ->re_char.c;
		else if ( templ->re_type == RE_TYPE_ANY_CHAR )
			;
		else if ( templ->re_type == RE_TYPE_SET )
			memcpy(copy->re_set.set, templ->re_set.set, 32);
		else if ( templ->re_type == RE_TYPE_SUBEXPRESSION )
		{
			copy->re_subexpression.index = templ->re_subexpression.index;
			templ->re_upcoming_state_next = parent_templ;
			copy->re_upcoming_state_next = parent_copy;
			parent_templ = templ;
			parent_copy = copy;
			templ = templ->re_subexpression.re_owner;
			re_ptr = &copy->re_subexpression.re_owner;
			continue;
		}
		else if ( templ->re_type == RE_TYPE_SUBEXPRESSION_END )
			copy->re_subexpression.index = templ->re_subexpression.index;
		else if ( templ->re_type == RE_TYPE_ALTERNATIVE ||
			      templ->re_type == RE_TYPE_OPTIONAL ||
			      templ->re_type == RE_TYPE_LOOP )
		{
			templ->re_upcoming_state_next = parent_templ;
			copy->re_upcoming_state_next = parent_copy;
			parent_templ = templ;
			parent_copy = copy;
			templ = templ->re_split.re_owner;
			re_ptr = &copy->re_split.re_owner;
			continue;
		}
		else if ( templ->re_type == RE_TYPE_REPETITION )
		{
			copy->re_repetition.min = templ->re_repetition.min;
			copy->re_repetition.max = templ->re_repetition.max;
			templ->re_upcoming_state_next = parent_templ;
			copy->re_upcoming_state_next = parent_copy;
			parent_templ = templ;
			parent_copy = copy;
			templ = templ->re_split.re;
			re_ptr = &copy->re_split.re;
			continue;
		}
		else
			assert(false);
		templ = templ->re_next_owner;
		re_ptr = &copy->re_next_owner;
	}
}

static inline bool re_repetition(struct re* templ,
                                 struct re** re_ptr,
                                 size_t min,
                                 size_t max,
                                 struct re* after)
{
	while ( true )
	{
		if ( !max )
			return *re_ptr = after, true;
		struct re* copy = (struct re*) calloc(1, sizeof(struct re));
		if ( !copy )
			return false;
		*re_ptr = copy;
		copy->re_type = templ->re_type;
		if ( templ->re_type == RE_TYPE_BOL )
			;
		else if ( templ->re_type == RE_TYPE_BOL )
			;
		else if ( templ->re_type == RE_TYPE_CHAR )
			copy->re_char.c = templ->re_char.c;
		else if ( templ->re_type == RE_TYPE_ANY_CHAR )
			;
		else if ( templ->re_type == RE_TYPE_SET )
			memcpy(copy->re_set.set, templ->re_set.set, 32);
		else if ( templ->re_type == RE_TYPE_SUBEXPRESSION )
		{
			copy->re_subexpression.index = templ->re_subexpression.index;
			if ( !re_duplicate(templ->re_subexpression.re_owner,
				              &copy->re_subexpression.re_owner) )
				return false;
			struct re* templ_end = templ->re_next_owner;
			assert(templ_end && templ_end->re_type == RE_TYPE_SUBEXPRESSION_END);
			struct re* end = (struct re*) calloc(1, sizeof(struct re));
			if ( !end )
				return false;
			end->re_type = RE_TYPE_SUBEXPRESSION_END;
			end->re_subexpression.index = templ_end->re_subexpression.index;
			copy->re_next_owner = end;
		}
		else
			assert(false);
		if ( 1 <= min )
		{
			while ( copy->re_next_owner )
				copy = copy->re_next_owner;
			re_ptr = &copy->re_next_owner;
			if ( max != SIZE_MAX )
				max--;
			min--;
		}
		else if ( max < SIZE_MAX )
		{
			struct re* wrap = (struct re*) calloc(1, sizeof(struct re));
			if ( !wrap )
				return false;
			wrap->re_type = RE_TYPE_OPTIONAL;
			wrap->re_split.re_owner = copy;
			*re_ptr = wrap;
			re_ptr = &wrap->re_next_owner;
			max--;
		}
		else
		{
			struct re* wrap = (struct re*) calloc(1, sizeof(struct re));
			if ( !wrap )
				return false;
			wrap->re_type = RE_TYPE_LOOP;
			wrap->re_split.re_owner = copy;
			*re_ptr = wrap;
			re_ptr = &wrap->re_next_owner;
			max = 0;
		}
	}
}

static inline bool re_transform(struct re** re_ptr, size_t* state_count_ptr)
{
	if ( !*re_ptr )
	{
		struct re* re;
		if ( !(re = (struct re*) calloc(1, sizeof(struct re))) )
			return false;
		re->re_type = RE_TYPE_BOL;
		*re_ptr = re;
	}

	struct re** parent_ptr = NULL;
	while ( *re_ptr )
	{
		struct re* re = *re_ptr;

		if ( re->re_type == RE_TYPE_REPETITION )
		{
			struct re* templ = re->re_repetition.re;
			size_t min = re->re_repetition.min;
			size_t max = re->re_repetition.max;
			struct re* after = re->re_next_owner;
			struct re* replacement = NULL;
			re->re_next_owner = NULL;
			re_repetition(templ, &replacement, min, max, after);
			re_free(re);
			*re_ptr = re = replacement;
			continue;
		}

		(*state_count_ptr)++;

		if ( re->re_type == RE_TYPE_SUBEXPRESSION &&
		     re->re_subexpression.re_owner )
		{
			re->re_current_state_prev = (struct re*) parent_ptr;
			parent_ptr = re_ptr;
			re_ptr = &re->re_subexpression.re_owner;
			continue;
		}

		if ( (re->re_type == RE_TYPE_ALTERNATIVE ||
		      re->re_type == RE_TYPE_OPTIONAL ||
		      re->re_type == RE_TYPE_LOOP) && re->re_split.re_owner )
		{
			re->re_current_state_prev = (struct re*) parent_ptr;
			parent_ptr = re_ptr;
			re_ptr = &re->re_split.re_owner;
			continue;
		}

		re_ptr = &re->re_next_owner;
		while ( !*re_ptr && parent_ptr )
		{
			re_ptr = parent_ptr;
			parent_ptr = (struct re**) (*re_ptr)->re_current_state_prev;
			re_ptr = &(*re_ptr)->re_next_owner;
		}
	}

	return true;
}

static inline void re_control_flow(struct re* re,
                                   regmatch_t* matches,
                                   size_t matches_per_state,
                                   size_t* state_count_ptr)
{
	struct re* parent = NULL;
	struct re* parent_link = NULL;
	while ( re )
	{
		size_t re_index = (*state_count_ptr)++;
		size_t offset = re_index * matches_per_state;
		re->re_matches = matches + offset;

		if ( re->re_type == RE_TYPE_ALTERNATIVE )
		{
			if ( !re->re_split.re_owner )
				re->re_split.re = parent_link;
			if ( !re->re_next_owner )
				re->re_next = parent_link;
			if ( re->re_split.re_owner && re->re_next_owner )
			{
				re->re_next = re->re_next_owner;
				re->re_current_state_prev = parent;
				re->re_current_state_next = parent_link;
				re->re_upcoming_state_next = re->re_next_owner;
				parent = re;
				re = re->re_split.re = re->re_split.re_owner;
			}
			else if ( re->re_split.re_owner )
				re = re->re_split.re = re->re_split.re_owner;
			else if ( re->re_next_owner )
				re = re->re_next = re->re_next_owner;
			else if ( parent )
			{
				re = parent;
				parent = re->re_current_state_prev;
				parent_link = re->re_current_state_next;
				re = re->re_upcoming_state_next;
			}
			else
				re = NULL;
			continue;
		}

		if ( !re->re_next_owner && parent_link )
			re->re_next = parent_link;
		else
			re->re_next = re->re_next_owner;

		if ( re->re_type == RE_TYPE_LOOP || re->re_type == RE_TYPE_OPTIONAL )
		{
			struct re* inner = re->re_split.re_owner;
			struct re* after = re->re_next;
			re->re_split.re = after;
			re->re_next = inner;
			if ( re->re_next_owner )
			{
				re->re_current_state_prev = parent;
				re->re_current_state_next = parent_link;
				re->re_upcoming_state_next = after;
				parent = re;
			}
			if ( re->re_type == RE_TYPE_LOOP )
				parent_link = re;
			else
				parent_link = after;
			re = inner;
			continue;
		}

		if ( re->re_type == RE_TYPE_SUBEXPRESSION )
		{
			if ( re->re_subexpression.re_owner )
			{
				re->re_current_state_prev = parent;
				re->re_current_state_next = parent_link;
				re->re_upcoming_state_next = re->re_next_owner;
				parent = re;
				parent_link = re->re_next;
				re->re_next = re->re_subexpression.re_owner;
				re = re->re_subexpression.re_owner;
				continue;
			}
		}

		if ( !re->re_next_owner && parent )
		{
			re = parent;
			parent = re->re_current_state_prev;
			parent_link = re->re_current_state_next;
		}

		re = re->re_next_owner;
	}
}

int regcomp(regex_t* restrict regex,
            const char* restrict pattern,
            int cflags)
{
	// TODO: Verify cflags.
	// TODO: Implement REG_ICASE.
	// TODO: Implement REG_NOSUB.
	// TODO: Implement REG_NEWLINE.
	memset(regex, 0, sizeof(*regex));
	pthread_mutex_init(&regex->re_lock, NULL);
	regex->re_cflags = cflags;
	struct re_parse parse;
	memset(&parse, 0, sizeof(parse));
	parse.subexpr_num = 1;
	int ret = re_parse(&parse, &regex->re, pattern, cflags);
	while ( parse.subexpr )
	{
		struct re_parse_subexpr* todelete = parse.subexpr;
		parse.subexpr = todelete->next;
		free(todelete);
	}
	if ( ret != 0 )
		return regfree(regex), ret;
	size_t state_count = 0;
	if ( !re_transform(&regex->re, &state_count) )
		return regfree(regex), REG_ESPACE;
	size_t matches_length;
	if ( __builtin_mul_overflow(parse.subexpr_num, state_count, &matches_length) )
		return regfree(regex), REG_ESPACE;
	regex->re_matches = (regmatch_t*)
		reallocarray(NULL, matches_length, sizeof(regmatch_t));
	if ( !regex->re_matches )
		return regfree(regex), REG_ESPACE;
	size_t state_recount = 0;
	re_control_flow(regex->re, regex->re_matches, parse.subexpr_num, &state_recount);
	assert(state_count == state_recount);
	if ( !(cflags & REG_NOSUB) )
		regex->re_nsub = parse.subexpr_num - 1;
	return ret;
}
