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
 * rules.c
 * Determines whether a given path is included in the filesystem.
 */

#include <ctype.h>
#include <errno.h>
#include <error.h>
#include <libgen.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "rules.h"

static struct InclusionRule** rules;
static size_t num_rules;
static size_t num_rules_allocated;
static bool default_inclusion = true;
static bool default_inclusion_determined;
static char** manifest;
static size_t manifest_used;
static size_t manifest_length;

static const char* SkipCharacters(const char* str, char c)
{
	while ( *str == c)
		str++;
	return str;
}

// /usr/bin/foobar match /usr = true
// /usr/bin/foobar match usr = false
// ///usr////bin//foobar match //usr// = true
// ///usr////bin//foobar match //usr//./evince = false
// TODO: Should this support . and .. too?
static bool PathMatchesPattern(const char* path, const char* pattern)
{
	bool last_was_slash = false;
	while ( true )
	{
		if ( !*pattern )
			return !*path || last_was_slash;
		if ( (last_was_slash = *pattern == '/') )
		{
			if ( *path == '/'  )
			{
				path = SkipCharacters(path, '/');
				pattern = SkipCharacters(pattern, '/');
				continue;
			}
			return false;
		}
		if ( *pattern++ != *path++ )
			return false;
	}
}

static int search_path(const void* a_ptr, const void* b_ptr)
{
	const char* key = (const char*) a_ptr;
	char* path = *(char**) b_ptr;
	return strcmp(key, path);
}

bool IncludesPath(const char* path)
{
	bool determined = false;
	bool included = false;
	for ( size_t i = 0; i < num_rules; i++ )
	{
		struct InclusionRule* rule = rules[i];
		if ( !PathMatchesPattern(path, rule->pattern) )
			continue;
		switch ( rules[i]->rule )
		{
		case RULE_INCLUDE:
			included = true;
			determined = true;
			break;
		case RULE_EXCLUDE:
			included = false;
			determined = true;
			break;
		}
	}
	if ( !determined )
		included = default_inclusion;
	if ( !included )
		return false;
	if ( manifest_used &&
	     !bsearch(path, manifest, manifest_used, sizeof(char*), search_path) )
		return false;
	return true;
}

bool ChangeRulesAmount(size_t new_length)
{
	size_t new_num_rules = new_length < num_rules ? new_length : num_rules;
	for ( size_t i = new_num_rules; i < num_rules; i++ )
	{
		free(rules[i]->pattern);
		free(rules[i]);
	}
	num_rules = new_num_rules;
	struct InclusionRule** new_rules = (struct InclusionRule**)
		malloc(sizeof(struct InclusionRule*) * new_length);
	for ( size_t i = 0; i < new_length && i < num_rules; i++ )
		new_rules[i] = rules[i];
	free(rules); rules = new_rules;
	num_rules_allocated = new_length;
	return true;
}

bool AddRule(struct InclusionRule* rule)
{
	if ( num_rules == num_rules_allocated )
	{
		size_t new_length = num_rules_allocated ? 2 * num_rules_allocated : 32;
		if ( !ChangeRulesAmount(new_length) )
			return false;
	}
	rules[num_rules++] = rule;
	return true;
}

static const char* SkipWhitespace(const char* line)
{
	while ( *line && isspace((unsigned char) *line) )
		line++;
	return line;
}

static bool IsLineComment(const char* line)
{
	return !*line || *line == '#';
}

static const char* IsLineCommand(const char* line, const char* command)
{
	while ( *line && isspace((unsigned char) *line) )
		line++;
	size_t cmdlen = strlen(command);
	if ( strncmp(line, command, cmdlen) != 0 )
		return NULL;
	if ( line[cmdlen] && !isspace((unsigned char) line[cmdlen]) )
		return NULL;
	while ( line[cmdlen] && isspace((unsigned char) line[cmdlen]) )
		cmdlen++;
	return line + cmdlen;
}

bool AddRulesFromFile(FILE* fp, const char* fpname)
{
	size_t rules_at_start = num_rules;
	size_t line_size;
	size_t line_num = 0;
	char* mem = NULL;
	ssize_t line_len;
	while ( 0 <= (line_len = getline(&mem, &line_size, fp)) )
	{
		char* line = mem;
		line_num++;
		if ( line_len && line[line_len-1] == '\n' )
			line[line_len-1] = '\0';
		line = (char*) SkipWhitespace((char*) line);
		if ( IsLineComment(line) )
			continue;
		const char* parameter;
		if ( (parameter = IsLineCommand(line, "default")) )
		{
			bool value;
			if ( !strcmp(parameter, "true") )
				value = true;
			else if ( !strcmp(parameter, "false") )
				value = false;
			else
			{
				error(0, 0, "%s:%zu: not a boolean '%s'", fpname,
				         line_num, parameter);
				goto error_out;
			}
			if ( !default_inclusion_determined )
				default_inclusion = value,
				default_inclusion_determined = true;
			else
				default_inclusion = default_inclusion || value;
		}
		else if ( (parameter = IsLineCommand(line, "exclude")) ||
		          (parameter = IsLineCommand(line, "include")) )
		{
			if ( !*parameter )
			{
				error(0, 0, "%s:%zu: no parameter given", fpname,
				         line_num);
				goto error_out;
			}
			const char* pattern = parameter;
			enum InclusionRuleType type = line[0] == 'e' ? RULE_EXCLUDE : RULE_INCLUDE;
			struct InclusionRule* rule =
				(struct InclusionRule*) malloc(sizeof(struct InclusionRule));
			rule->pattern = strdup(pattern);
			rule->rule = type;
			if ( !AddRule(rule) )
				goto error_out_errno;
		}
		else
		{
			error(0, 0, "%s:%zu: line not understood: '%s'", fpname,
			         line_num, line);
			goto error_out;
		}
	}
	free(mem);
	if ( ferror(fp) )
	{
	error_out_errno:
		error(0, errno, "%s", fpname);
	error_out:
		ChangeRulesAmount(rules_at_start);
		return false;
	}
	return true;
}

int compare_path(const void* a_ptr, const void* b_ptr)
{
	const char* a = *(const char* const*) a_ptr;
	const char* b = *(const char* const*) b_ptr;
	return strcmp(a, b);
}

bool AddManifestPath(const char* path)
{
	if ( manifest_used == manifest_length )
	{
		size_t new_length = 2 * manifest_length;
		if ( new_length == 0 )
			new_length = 64;
		size_t new_size = new_length * sizeof(char*);
		char** new_manifest = (char**) realloc(manifest, new_size);
		if ( !new_manifest )
		{
			error(0, errno, "malloc");
			return false;
		}
		manifest = new_manifest;
		manifest_length = new_length;
	}
	char* copy = strdup(path);
	if ( !copy )
	{
		error(0, errno, "malloc");
		return false;
	}
	manifest[manifest_used++] = copy;
	return true;
}

bool AddManifestFromFile(FILE* fp, const char* fpname)
{
	char* line = NULL;
	size_t line_size = 0;
	ssize_t line_len;
	while ( 0 <= (line_len = getline(&line, &line_size, fp)) )
	{
		if ( line_len && line[line_len-1] == '\n' )
			line[line_len-1] = '\0';
		if ( !AddManifestPath(line) )
			return false;
	}
	free(line);
	if ( ferror(fp) )
	{
		error(0, errno, "%s", fpname);
		return false;
	}
	if ( !AddManifestPath("/") ||
	     !AddManifestPath("/tix") ||
	     !AddManifestPath("/tix/manifest") )
		return false;
	char* fpname_copy = strdup(fpname);
	if ( !fpname_copy )
	{
		error(0, errno, "malloc");
		return false;
	}
	const char* fpname_basename = basename(fpname_copy);
	char* manifest_path;
	if ( asprintf(&manifest_path, "/tix/manifest/%s", fpname_basename) < 0 )
	{
		free(fpname_copy);
		error(0, errno, "malloc");
		return false;
	}
	free(fpname_copy);
	if ( !AddManifestPath(manifest_path) )
		return free(manifest_path), false;
	free(manifest_path);
	qsort(manifest, manifest_used, sizeof(char*), compare_path);
	return true;
}
