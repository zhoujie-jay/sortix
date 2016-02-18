/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2015, 2016.

    This program is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
    more details.

    You should have received a copy of the GNU General Public License along with
    this program. If not, see <http://www.gnu.org/licenses/>.

    release.c
    Utility functions to handle release information.

*******************************************************************************/

#include <err.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "release.h"

void release_free(struct release* release)
{
	free(release->pretty_name);
}

static char* os_release_eval(const char* string)
{
	char* result;
	size_t result_size;
	FILE* fp = open_memstream(&result, &result_size);
	if ( !fp )
		return NULL;
	bool escaped = false;
	bool singly_quote = false;
	bool doubly_quote = false;
	while ( *string )
	{
		char c = *string++;
		if ( !escaped && !singly_quote && c == '\\' )
			escaped = true;
		else if ( !escaped && !doubly_quote && c == '\'' )
			singly_quote = !singly_quote;
		else if ( !escaped && !singly_quote && c == '"' )
			doubly_quote = !doubly_quote;
		else
		{
			fputc((unsigned char) c, fp);
			escaped = false;
		}
	}
	if ( fclose(fp) == EOF )
	{
		free(result);
		return NULL;
	}
	return result;
}

static void parse_version(const char* string,
                          unsigned long* major_ptr,
                          unsigned long* minor_ptr)

{
	*major_ptr = strtoul(string, (char**) &string, 10);
	if ( *string == '.' )
		string++;
	*minor_ptr = strtoul(string, (char**) &string, 10);
}

static void parse_release(const char* string,
                          unsigned long* major_ptr,
                          unsigned long* minor_ptr,
                          bool* dev_ptr)

{
	*major_ptr = strtoul(string, (char**) &string, 10);
	if ( *string == '.' )
		string++;
	*minor_ptr = strtoul(string, (char**) &string, 10);
	*dev_ptr = *string != '\0';
}

bool os_release_load(struct release* release,
                     const char* path,
                     const char* errpath)
{
	memset(release, 0, sizeof(*release));
	FILE* fp = fopen(path, "r");
	if ( !fp )
	{
		if ( errno != ENOENT )
			warn("%s", errpath);
		return false;
	}
	bool failure = false;
	bool success = false;
	bool found_id = false;
	bool found_pretty_name = false;
	bool found_sortix_abi = false;
	bool found_version_id = false;
	char* line = NULL;
	size_t line_size = 0;
	ssize_t line_length;
	while ( 0 < (errno = 0, line_length = getline(&line, &line_size, fp)) )
	{
		if ( line[line_length-1] == '\n' )
			line[--line_length] = '\0';
		if ( !strncmp(line, "ID=", strlen("ID=")) )
		{
			const char* param = line + strlen("ID=");
			found_id = true;
			char* value = os_release_eval(param);
			if ( value )
			{
				if ( strcmp(value, "sortix") != 0 )
				{
					warn("%s: ID does not specify a 'sortix' system", errpath);
					failure = true;
				}
				free(value);
			}
			else
			{
				warn("malloc");
				failure = true;
			}
		}
		else if ( !strncmp(line, "PRETTY_NAME=", strlen("PRETTY_NAME=")) )
		{
			const char* param = line + strlen("PRETTY_NAME=");
			found_pretty_name = true;
			if ( !(release->pretty_name = os_release_eval(param)) )
			{
				warn("malloc");
				failure = true;
			}
		}
		else if ( !strncmp(line, "SORTIX_ABI=", strlen("SORTIX_ABI=")) )
		{
			const char* param = line + strlen("SORTIX_ABI=");
			found_sortix_abi = true;
			char* value = os_release_eval(param);
			if ( value )
			{
				parse_version(value, &release->abi_major, &release->abi_minor);
				free(value);
			}
			else
			{
				warn("malloc");
				failure = true;
			}
		}
		else if ( !strncmp(line, "VERSION_ID=", strlen("VERSION_ID=")) )
		{
			const char* param = line + strlen("VERSION_ID=");
			found_version_id = true;
			char* value = os_release_eval(param);
			if ( value )
			{
				parse_release(value, &release->version_major,
				              &release->version_minor, &release->version_dev);
				free(value);
			}
			else
			{
				warn("malloc");
				failure = true;
			}
		}
	}
	if ( errno )
		warn("%s", errpath);
	else if ( failure )
		;
	else if ( !found_id )
		warnx("%s: No ID", errpath);
	else if ( !found_pretty_name )
		warnx("%s: No PRETTY_NAME", errpath);
	else if ( !found_sortix_abi )
		warnx("%s: No SORTIX_ABI", errpath);
	else if ( !found_version_id )
		warnx("%s: No VERSION_ID", errpath);
	else
		success = true;
	free(line);
	fclose(fp);
	if ( failure || !success )
	{
		release_free(release);
		return false;
	}
	return true;
}
