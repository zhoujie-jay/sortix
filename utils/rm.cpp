/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012.

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

    rm.cpp
    Remove files or directories.

*******************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>

#include <dirent.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if !defined(VERSIONSTR)
#define VERSIONSTR "unknown version"
#endif

// TODO: Wrong implementation of -f! It means ignore *nonexistent files*, not
// ignore all errors!

int DupHandleCWD(int fd)
{
	if ( fd == AT_FDCWD )
		return open(".", O_RDONLY);
	return dup(fd);
}

bool RemoveRecursively(int dirfd, const char* full, const char* rel,
                       bool force, bool verbose)
{
	if ( unlinkat(dirfd, rel, 0) == 0 )
	{
		if ( verbose )
			fprintf(stderr, "removed `%s'\n", full);
		return true;
	}
	if ( errno != EISDIR )
	{
		if ( force && errno == ENOENT )
			return true;
		error(0, errno, "cannot remove: %s", full);
		return false;
	}
	if ( unlinkat(dirfd, rel, AT_REMOVEDIR) == 0 )
	{
		if ( verbose )
			fprintf(stderr, "removed `%s'\n", full);
		return true;
	}
	if ( errno != ENOTEMPTY )
	{
		if ( force && errno == ENOENT )
			return true;
		error(0, errno, "cannot remove: %s", full);
		return false;
	}
	int targetfd = openat(dirfd, rel, O_RDONLY | O_DIRECTORY);
	if ( targetfd < 0 )
	{
		if ( force && errno == ENOENT )
			return true;
		error(0, errno, "cannot open: %s", full);
		return false;
	}
	DIR* dir = fdopendir(targetfd);
	if ( !dir )
	{
		error(0, errno, "fdopendir");
		close(targetfd);
		return false;
	}
	bool ret = true;
	while ( struct dirent* entry = readdir(dir) )
	{
		const char* name = entry->d_name;
		if ( strcmp(name, ".") == 0 || strcmp(name, "..") == 0 )
			continue;
		char* newfull = new char[strlen(full) + 1 + strlen(entry->d_name) + 1];
		bool addslash = !full[0] || full[strlen(full)-1] != '/';
		stpcpy(stpcpy(stpcpy(newfull, full), addslash ? "/" : ""), name);
		bool ok = RemoveRecursively(targetfd, newfull, name, false, verbose);
		delete[] newfull;
		rewinddir(dir);
		if ( !ok ) { ret = false; break; }
	}
	closedir(dir);
	if ( ret )
	{
		if ( unlinkat(dirfd, rel, AT_REMOVEDIR) == 0 )
		{
			if ( verbose )
				fprintf(stderr, "removed `%s'\n", full);
		}
		else
		{
			if ( !force || errno == ENOENT )
			{
				error(0, errno, "cannot remove: %s", full);
				ret = false;
			}
		}
	}
	return ret;
}

void Help(FILE* fp, const char* argv0)
{
	fprintf(fp, "Usage: %s [OPTION]... FILE...\n", argv0);
	fprintf(fp, "Remove files by unlinking their directory entries.\n");
}

void Version(FILE* fp, const char* argv0)
{
	fprintf(fp, "%s (Sortix) %s\n", argv0, VERSIONSTR);
	fprintf(fp, "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n");
	fprintf(fp, "This is free software: you are free to change and redistribute it.\n");
	fprintf(fp, "There is NO WARRANTY, to the extent permitted by law.\n");
}

int main(int argc, char* argv[])
{
	const char* argv0 = argv[0];
	bool force = false;
	bool recursive = false;
	bool verbose = false;
	for ( int i = 1; i < argc; i++ )
	{
		const char* arg = argv[i];
		if ( arg[0] != '-' )
			continue;
		argv[i] = NULL;
		if ( !strcmp(arg, "--") )
			break;
		if ( arg[1] != '-' )
			for ( size_t i = 1; arg[i]; i++ )
				switch ( arg[i] )
				{
				case 'f': force = true; break;
				case 'r': recursive = true; break;
				case 'R': recursive = true; break;
				case 'v': verbose = true; break;
				default:
					fprintf(stderr, "%s: unknown option -- '%c'\n", argv0, arg[i]);
					Help(stderr, argv0);
					exit(1);
				}
		else if ( !strcmp(arg, "--force") )
			force = true;
		else if ( !strcmp(arg, "--recursive") )
			recursive = true;
		else if ( !strcmp(arg, "--verbose") )
			verbose = true;
		else if ( !strcmp(arg, "--help") ) { Help(stdout, argv0); exit(0); }
		else if ( !strcmp(arg, "--version") ) { Version(stdout, argv0); exit(0); }
		else
		{
			fprintf(stderr, "%s: unknown option: %s\n", argv0, arg);
			Help(stderr, argv0);
			exit(1);
		}
	}

	int num_args = 0;
	for ( int i = 1; i < argc; i++ )
		if ( argv[i] )
			num_args++;

	if ( !num_args && !force )
	{
		fprintf(stderr, "%s: missing operand\n", argv0);
		exit(1);
	}

	int main_ret = 0;
	for ( int i = 1; i < argc; i++ )
	{
		const char* arg = argv[i];
		if ( !arg )
			continue;
		if ( unlink(arg) < 0 )
		{
			if ( !recursive || errno != EISDIR )
			{
				if ( force && errno == ENOENT )
					continue;
				error(0, errno, "cannot remove: %s", arg);
				main_ret = 1;
				continue;
			}
			if ( !RemoveRecursively(AT_FDCWD, arg, arg, force, verbose) )
			{
				main_ret = 1;
				continue;
			}
		}
		else
		{
			if ( verbose )
				fprintf(stderr, "removed `%s'\n", arg);
		}
	}

	return main_ret;
}
