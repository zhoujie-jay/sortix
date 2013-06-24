/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012.

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

    stdlib/env.cpp
    Environmental variables utilities.

*******************************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

/* Since legacy applications rely on being able to modify the environ variable,
   we have to keep track of whether it is what we expect and whether we know the
   size of it. If it changed, we need to recount, as well as malloc a copy of it
   if we wish to make changes. */
extern "C" { char** environ = NULL; }
static char** environ_malloced = NULL;
static char** environ_counted = NULL;
size_t environlen = 0;
size_t environroom = 0;

static inline bool environismalloced()
{
	return environ && environ == environ_malloced;
}

extern "C" const char* const* getenviron(void)
{
	return environ;
}

extern "C" size_t envlength(void)
{
	if ( !environ ) { return 0; }
	if ( environ_counted != environ )
	{
		size_t count = 0;
		while ( environ[count] ) { count++; }
		environ_counted = environ;
		environlen = count;
	}
	return environlen;
}

extern "C" const char* getenvindexed(size_t index)
{
	if ( envlength() <= index ) { errno = EBOUND; return NULL; }
	return environ[index];
}

extern "C" const char* sortix_getenv(const char* name)
{
	size_t equalpos = strcspn(name, "=");
	if ( name[equalpos] == '=' ) { return NULL; }
	size_t namelen = equalpos;
	size_t envlen = envlength();
	for ( size_t i = 0; i < envlen; i++ )
	{
		if ( strncmp(name, environ[i], namelen) ) { continue; }
		if ( environ[i][namelen] != '=' ) { continue; }
		return environ[i] + namelen + 1;
	}
	return NULL;
}

extern "C" char* getenv(const char* name)
{
	return (char*) sortix_getenv(name);
}

static bool makeenvironmalloced()
{
	if ( environismalloced() ) { return true; }
	size_t envlen = envlength();
	size_t newenvlen = envlen;
	size_t newenvsize = sizeof(char*) * (newenvlen+1);
	char** newenviron = (char**) malloc(newenvsize);
	if ( !newenviron ) { return false; }
	size_t sofar = 0;
	for ( size_t i = 0; i < envlen; i++ )
	{
		newenviron[i] = strdup(environ[i]);
		if ( !newenviron[i] ) { goto cleanup; }
		sofar = i;
	}
	newenviron[envlen] = NULL;
	environlen = environroom = newenvlen;
	environ = environ_malloced = environ_counted = newenviron;
	return true;
cleanup:
	for ( size_t i = 0; i < sofar; i++ ) { free(newenviron[i]); }
	free(newenviron);
	return false;
}

extern "C" int clearenv(void)
{
	if ( !environ ) { return 0; }
	if ( environismalloced() )
	{
		for ( char** varp = environ; *varp; varp++ ) { free(*varp); }
		free(environ);
	}
	environ = environ_counted = environ_malloced = NULL;
	return 0;
}

static bool doputenv(char* str, char* freeme, bool overwrite)
{
	if ( !makeenvironmalloced() ) { free(freeme); return false; }
	size_t strvarnamelen = strcspn(str, "=");
	for ( size_t i = 0; i < envlength(); i++ )
	{
		char* var = environ[i];
		if ( strncmp(str, var, strvarnamelen) ) { continue; }
		if ( !overwrite ) { free(freeme); return true; }
		free(var);
		environ[i] = str;
		return true;
	}
	if ( environlen == environroom )
	{
		size_t newenvironroom = environroom ? 2 * environroom : 16;
		size_t newenvironsize = sizeof(char*) * (newenvironroom+1);
		char** newenviron = (char**) realloc(environ, newenvironsize);
		if ( !newenviron ) { free(freeme); return false; }
		environ = environ_malloced = environ_counted = newenviron;
		environroom = newenvironroom;
	}
	environ[environlen++] = str;
	environ[environlen] = NULL;
	return true;
}

extern "C" int setenv(const char* name, const char* value, int overwrite)
{
	if ( !name || !name[0] || strchr(name, '=') ) { errno = EINVAL; return -1; }
	char* str = (char*) malloc(strlen(name) + 1 + strlen(value) + 1);
	if ( !str ) { return -1; }
	stpcpy(stpcpy(stpcpy(str, name), "="), value);
	if ( !doputenv(str, str, overwrite) ) { return -1; }
	return 0;
}

extern "C" int putenv(char* str)
{
	if ( !strchr(str, '=') ) { errno = EINVAL; return -1; }
	// TODO: HACK: This voilates POSIX as it mandates that callers must be able
	// to modify the environment by modifying the string. However, this is bad
	// design! It also means we got a memory leak since we can't safely free
	// strings from the environment when overriding them. Therefore we create
	// a copy of the strings here and use the copy instead. This is a problem
	// for some applications that will subtly break.
	char* strcopy = strdup(str);
	if ( !strcopy )
		return -1;
	if ( !doputenv(strcopy, strcopy, true) ) { return -1; }
	return 0;
}

extern "C" int unsetenv(const char* name)
{
	size_t equalpos = strcspn(name, "=");
	if ( name[equalpos] == '=' ) { return 0; }
	size_t namelen = equalpos;
	size_t envlen = envlength();
	for ( size_t i = 0; i < envlen; i++ )
	{
		if ( strncmp(name, environ[i], namelen) ) { continue; }
		if ( environ[i][namelen] != '=' ) { continue; }
		if ( environismalloced() )
		{
			char* str = environ[i];
			free(str);
		}
		if ( i < envlen-1 ) { environ[i] = environ[envlen-1]; }
		environ[--environlen] = NULL;
		return 0;
	}
	return 0;
}
