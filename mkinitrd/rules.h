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

    rules.h
    Determines whether a given path is included in the filesystem.

*******************************************************************************/

#ifndef RULES_H
#define RULES_H

enum InclusionRuleType
{
	RULE_INCLUDE,
	RULE_EXCLUDE,
};

class InclusionRule
{
public:
	InclusionRule(const char* pattern, InclusionRuleType rule);
	~InclusionRule();
	bool MatchesPath(const char* path) const;

public:
	char* pattern;
	InclusionRuleType rule;

};

class InclusionRules
{
public:
	InclusionRules();
	~InclusionRules();
	bool IncludesPath(const char* path) const;
	bool AddRule(InclusionRule* rule);
	bool AddRulesFromFile(FILE* fp, FILE* err, const char* fpname);
	bool AddManifestFromFile(FILE* fp, FILE* err, const char* fpname);
	bool AddManifestPath(const char* path, FILE* err);
	bool ChangeRulesAmount(size_t newnum);

public:
	InclusionRule** rules;
	size_t num_rules;
	size_t num_rules_allocated;
	bool default_inclusion;
	bool default_inclusion_determined;
	char** manifest;
	size_t manifest_used;
	size_t manifest_length;

};

#endif
