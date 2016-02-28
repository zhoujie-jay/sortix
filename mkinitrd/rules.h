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

struct InclusionRule
{
	char* pattern;
	enum InclusionRuleType rule;
};

bool IncludesPath(const char* path);
bool AddRule(struct InclusionRule* rule);
bool AddRulesFromFile(FILE* fp, const char* fpname);
bool AddManifestFromFile(FILE* fp, const char* fpname);
bool AddManifestPath(const char* path);
bool ChangeRulesAmount(size_t newnum);

#endif
