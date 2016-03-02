/*
 * Copyright (c) 2015, 2016 Jonas 'Sortie' Termansen.
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
 * conf.c
 * Utility functions to handle upgrade.conf(5).
 */

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "conf.h"

static bool conf_boolean(const char* name, const char* value, const char* path)
{
	if ( !strcmp(value, "yes") )
		return true;
	if ( !strcmp(value, "no") )
		return false;
	printf("%s: %s: Expected yes or no instead of unsupported value\n",
	       path, name);
	return false;
}

static void conf_assign(struct conf* conf,
                        const char* name,
                        const char* value,
                        const char* path)
{
	if ( !strcmp(name, "grub") )
		conf->grub = conf_boolean(name, value, path);
	else if ( !strcmp(name, "newsrc") )
		conf->newsrc = conf_boolean(name, value, path);
	else if ( !strcmp(name, "ports") )
		conf->ports = conf_boolean(name, value, path);
	else if ( !strcmp(name, "src") )
		conf->src = conf_boolean(name, value, path);
	else if ( !strcmp(name, "system") )
		conf->system = conf_boolean(name, value, path);
	else
		printf("%s: %s: Unsupported variable\n", path, name);
}

void load_upgrade_conf(struct conf* conf, const char* path)
{
	memset(conf, 0, sizeof(*conf));
	conf->ports = true;
	conf->system = true;

	FILE* fp = fopen(path, "r");
	if ( !fp )
	{
		if ( errno == ENOENT )
			return;
		err(2, "%s", path);
	}
	char* line = NULL;
	size_t line_size = 0;
	ssize_t line_length;
	while ( 0 < (errno = 0, line_length = getline(&line, &line_size, fp)) )
	{
		if ( line[line_length - 1] == '\n' )
			line[--line_length] = '\0';
		line_length = 0;
		while ( line[line_length] && line[line_length] != '#' )
			line_length++;
		line[line_length] = '\0';
		while ( line_length && isblank((unsigned char) line[line_length - 1]) )
			line[--line_length] = '\0';
		char* name = line;
		while ( *name && isblank((unsigned char) *name) )
			name++;
		if ( !*name || *name == '=' )
			continue;
		size_t name_length = 1;
		while ( name[name_length] &&
		        !isblank((unsigned char) name[name_length]) &&
		        name[name_length] != '=' )
			name_length++;
		char* value = name + name_length;
		while ( *value && isblank((unsigned char) *value) )
			value++;
		if ( *value != '=' )
			continue;
		value++;
		while ( *value && isblank((unsigned char) *value) )
			value++;
		name[name_length] = '\0';
		conf_assign(conf, name, value, path);
	}
	if ( errno )
		err(2, "%s", path);
	free(line);
	fclose(fp);
}
