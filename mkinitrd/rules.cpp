/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

    This file is part of Sortix.

    Sortix is free software: you can redistribute it and/or modify it under the
    terms of the GNU General Public License as published by the Free Software
    Foundation, either version 3 of the License, or (at your option) any later
    version.

    Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
    details.

    You should have received a copy of the GNU General Public License along
    with Sortix. If not, see <http://www.gnu.org/licenses/>.

    rules.cpp
    Determines whether a given path is included in the filesystem.

*******************************************************************************/

#include <ctype.h>
#include <errno.h>
#include <error.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "rules.h"

static void error_fp(FILE* fp, int status, int errnum, const char* format, ...)
{
	fprintf(fp, "%s: ", program_invocation_name);

	va_list list;
	va_start(list, format);
	vfprintf(fp, format, list);
	va_end(list);

	if ( errnum )
		fprintf(fp, ": %s", strerror(errnum));
	fprintf(fp, "\n");
	if ( status )
		exit(status);
}

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

InclusionRule::InclusionRule(const char* pattern, InclusionRuleType rule)
{
	this->pattern = strdup(pattern);
	this->rule = rule;
}

InclusionRule::~InclusionRule()
{
	free(pattern);
}

bool InclusionRule::MatchesPath(const char* path) const
{
	return PathMatchesPattern(path, pattern);
}

InclusionRules::InclusionRules()
{
	rules = NULL;
	num_rules = num_rules_allocated = 0;
	default_inclusion = true;
}

InclusionRules::~InclusionRules()
{
	for ( size_t i = 0; i < num_rules; i++ )
		delete rules[i];
	delete[] rules;
}

bool InclusionRules::IncludesPath(const char* path) const
{
	bool determined = false;
	bool included = false;
	for ( size_t i = 0; i < num_rules; i++ )
	{
		InclusionRule* rule = rules[i];
		if ( !rule->MatchesPath(path) )
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
	return included;
}

bool InclusionRules::ChangeRulesAmount(size_t new_length)
{
	size_t new_num_rules = new_length < num_rules ? new_length : num_rules;
	for ( size_t i = new_num_rules; i < num_rules; i++ )
		delete rules[i];
	num_rules = new_num_rules;
	InclusionRule** new_rules = new InclusionRule*[new_length];
	for ( size_t i = 0; i < new_length && i < num_rules; i++ )
		new_rules[i] = rules[i];
	delete[] rules; rules = new_rules;
	num_rules_allocated = new_length;
	return true;
}

bool InclusionRules::AddRule(InclusionRule* rule)
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
	while ( *line && isspace(*line) ) line++;
	return line;
}

static char* SkipWhitespace(char* line)
{
	return (char*) SkipWhitespace((const char*) line);
}

static bool IsLineComment(const char* line)
{
	return !*line || *line == '#';
}

static const char* IsLineCommand(const char* line, const char* command)
{
	while ( *line && isspace(*line) ) line++;
	size_t cmdlen = strlen(command);
	if ( strncmp(line, command, cmdlen) != 0 )
		return NULL;
	if ( line[cmdlen] && !isspace(line[cmdlen]) )
		return NULL;
	while ( line[cmdlen] && isspace(line[cmdlen]) )
		cmdlen++;
	return line + cmdlen;
}

bool InclusionRules::AddRulesFromFile(FILE* fp, FILE* err, const char* fpname)
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
		line = SkipWhitespace(line);
		if ( IsLineComment(line) )
			continue;
		const char* parameter;
		if ( (parameter = IsLineCommand(line, "default")) )
		{
			bool value;
			if ( !strcmp(parameter, "true") )
				value = true;
			else if ( !strcmp(parameter, "true") )
				value = false;
			else
			{
				error_fp(err, 0, 0, "%s:%zu: not a boolean '%s'", fpname,
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
				error_fp(err, 0, 0, "%s:%zu: no parameter given", fpname,
				         line_num);
				goto error_out;
			}
			const char* pattern = parameter;
			InclusionRuleType type = line[0] == 'e' ? RULE_EXCLUDE : RULE_INCLUDE;
			InclusionRule* rule = new InclusionRule(pattern, type);
			if ( !AddRule(rule) )
				goto error_out_errno;
		}
		else
		{
			error_fp(err, 0, 0, "%s:%zu: line not understood: '%s'", fpname,
			         line_num, line);
			goto error_out;
		}
	}
	free(mem);
	if ( ferror(fp) )
	{
	error_out_errno:
		error_fp(err, 0, errno, "%s", fpname);
	error_out:
		ChangeRulesAmount(rules_at_start);
		return false;
	}
	return true;
}
