/*
 * Copyright (c) 2013 Jonas 'Sortie' Termansen.
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
 * fnmatch/fnmatch.c
 * Filename matching.
 */

#include <errno.h>
#include <fnmatch.h>
#include <stdbool.h>
#include <stddef.h>

#define __FNM_NOT_LEADING (1 << 31)

// TODO: This doesn't properly handle multibyte sequences.
// TODO: This doesn't fully implement all the POSIX requirements.
static bool is_allowed_bracket_pattern(const char* pattern, int flags,
                                       const char** pattern_end)
{
	size_t pi = 0;
	if ( pattern[pi++] != '[' )
		return false;
	if ( pattern[pi] == '!' || pattern[pi] == '^' )
		pi++;
	bool escaped = false;
	while ( escaped || pattern[pi] != ']' )
	{
		if ( !pattern[pi] )
			return false;
		else if ( !escaped && pattern[pi] == '\\' )
			escaped = true;
		else
		{
			if ( (flags & FNM_PATHNAME) && pattern[pi] == '/' )
				return false;
			escaped = false;
		}
		pi++;
	}
	return *pattern_end = pattern + pi + 1, true;
}

// TODO: This doesn't properly handle multibyte sequences.
// TODO: This doesn't fully implement all the POSIX requirements.
static bool matches_bracket_pattern(char c, const char* pattern, int flags)
{
	if ( (flags & FNM_PATHNAME) && c == '/' )
		return false;
	size_t pi = 1;
	bool negated = (pattern[pi] == '!' && (pi++, true)) ||
	               (pattern[pi] == '^' && (pi++, true));
	if ( (flags & FNM_PERIOD) && c == '.' )
	{
		if ( negated && !(flags & __FNM_NOT_LEADING) )
			return false;
	}
	bool escaped = false;
	bool matched_any = false;
	while ( escaped || pattern[pi] != ']' )
	{
		if ( !escaped && pattern[pi] == '\\' )
			escaped = true;
		else if ( pattern[pi] == c )
		{
			if ( negated )
				return false;
			else
				matched_any = true;
			escaped = false;
		}
		else
			escaped = false;
		pi++;
	}
	return negated || matched_any;
}

int fnmatch(const char* pattern, const char* string, int flags)
{
	int next_flags = flags | __FNM_NOT_LEADING;
	const char* pattern_end;
	if ( !pattern[0] )
	{
		if ( !string[0] )
			return 0;
	}
	else if ( pattern[0] == '*' )
	{
		if ( fnmatch(pattern + 1, string, flags) == 0 )
			return 0;
		if ( (flags & FNM_PERIOD) && string[0] == '.' )
			if ( !(flags & __FNM_NOT_LEADING) )
				return FNM_NOMATCH;
		if ( (flags & FNM_PATHNAME) && string[0] == '/' )
			return FNM_NOMATCH;
		if ( !string[0] )
			return FNM_NOMATCH;
		return fnmatch(pattern, string + 1, next_flags);
	}
	else if ( !string[0] )
		return FNM_NOMATCH;
	else if ( is_allowed_bracket_pattern(pattern, flags, &pattern_end) )
	{
		if ( !matches_bracket_pattern(string[0], pattern, flags) )
			return FNM_NOMATCH;
		return fnmatch(pattern_end, string + 1, next_flags);
	}
	else if ( !(flags & FNM_NOESCAPE) && pattern[0] == '\\' )
	{
		if ( !pattern[1] )
			return errno = EINVAL, -1;
		if ( pattern[1] == string[0] )
		{
			if ( (flags & FNM_PATHNAME) && string[0] == '/' )
				next_flags &= ~__FNM_NOT_LEADING;
			return fnmatch(pattern + 2, string + 1, next_flags);
		}
	}
	else if ( pattern[0] == '?' )
	{
		if ( (flags & FNM_PERIOD) && string[0] == '.' )
			if ( !(flags & __FNM_NOT_LEADING) )
				return FNM_NOMATCH;
		if ( (flags & FNM_PATHNAME) && string[0] == '/' )
			return FNM_NOMATCH;
		return fnmatch(pattern + 1, string + 1, next_flags);
	}
	else if ( pattern[0] == string[0] )
	{
		if ( (flags & FNM_PATHNAME) && string[0] == '/' )
			next_flags &= ~__FNM_NOT_LEADING;
		return fnmatch(pattern + 1, string + 1, next_flags);
	}
	return FNM_NOMATCH;
}
