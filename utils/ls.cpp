/*******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011, 2012.

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

	ls.cpp
	Lists directory contents.

*******************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stddef.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>

bool longformat = false;
bool showdotdot = false;
bool showdotfiles = false;

pid_t childpid;

void finishoutput()
{
	int status;
	fflush(stdout);
	if ( childpid ) { close(1); wait(&status); }
	childpid = 0;
}

int handleentry(const char* path, const char* name)
{
	bool isdotdot = strcmp(name, ".") == 0 || strcmp(name, "..") == 0;
	bool isdotfile = !isdotdot && name[0] == '.';
	if ( isdotdot && !showdotdot ) { return 0; }
	if ( isdotfile && !showdotfiles ) { return 0; }
	if ( !longformat )
	{
		printf("%s\n", name);
		return 0;
	}
	// TODO: Use openat and fstat.
	char* fullpath = new char[strlen(path) + 1 + strlen(name) + 1];
	strcpy(fullpath, path);
	strcat(fullpath, "/");
	strcat(fullpath, name);
	struct stat st;
	if ( stat(fullpath, &st) )
	{
		finishoutput();
		error(0, errno, "stat: %s", fullpath);
		return 2;
	}
	char perms[11];
	perms[0] = '?';
	if ( S_ISREG(st.st_mode) ) { perms[0] = '-'; }
	if ( S_ISDIR(st.st_mode) ) { perms[0] = 'd'; }
	const char flagnames[] = { 'x', 'w', 'r' };
	for ( size_t i = 0; i < 9; i++ )
	{
		bool set = st.st_mode & (1UL<<i);
		perms[9-i] = set ? flagnames[i % 3] : '-';
	}
	const char* sizeunit = "B";
	off_t size = st.st_size;
	if ( 1023 < size ) { size /= 1024UL; sizeunit = "K"; }
	if ( 1023 < size ) { size /= 1024UL; sizeunit = "M"; }
	if ( 1023 < size ) { size /= 1024UL; sizeunit = "G"; }
	if ( 1023 < size ) { size /= 1024UL; sizeunit = "T"; }
	if ( 1023 < size ) { size /= 1024UL; sizeunit = "P"; }
	perms[10] = 0;
	printf("%s %ji root root %ji%s\t%s\n", perms, (uintmax_t) st.st_nlink,
	       (uintmax_t) size, sizeunit, name);
	return 0;
}

int ls(const char* path)
{
	DIR* dir = opendir(path);
	if ( !dir ) { finishoutput(); error(0, errno, "%s", path); return 2; }

	struct dirent* entry;
	while ( (entry = readdir(dir)) )
	{
		int result;
		if ( (result = handleentry(path, entry->d_name)) ) { return result; }
	}

#if defined(sortix)
	if ( derror(dir) )
	{
		finishoutput();
		error(0, errno, "readdir: %s", path);
		return 2;
	}
#endif

	closedir(dir);

	return 0;
}

void usage(FILE* fp, const char* argv0)
{
	fprintf(fp, "usage: %s [-l] [-a | -A] [<DIR> ...]\n", argv0);
}

void version(FILE* fp, const char* argv0)
{
	return usage(fp, argv0);
}

void help(FILE* fp, const char* argv0)
{
	return usage(fp, argv0);
}

int main(int argc, char* argv[])
{
	const char* argv0 = argv[0];
	for ( int i = 0; i < argc; i++ )
	{
		const char* arg = argv[i];
		if ( arg[0] != '-' || !argv[1] ) { continue; }
		argv[i] = NULL;
		if ( !strcmp(arg, "--") ) { break; }
		if ( arg[1] != '-' )
		{
			char c;
			while ( (c = *++arg) )
			{
				switch ( c )
				{
				case 'l':
					longformat = true;
					break;
				case 'a':
					showdotdot = true;
					showdotfiles = true;
					break;
				case 'A':
					showdotdot = false;
					showdotfiles = true;
					break;
				default:
					fprintf(stderr, "%s: unknown option -- '%c'\n", argv0, c);
					usage(stderr, argv0);
					exit(2);
				}
			}
		}
		else if ( !strcmp(arg, "--version") )
		{
			version(stdout, argv0); exit(0);
		}
		else if ( !strcmp(arg, "--usage") )
		{
			usage(stdout, argv0); exit(0);
		}
		else if ( !strcmp(arg, "--help") )
		{
			help(stdout, argv0); exit(0);
		}
		else
		{
			fprintf(stderr, "%s: unrecognized option: %s\n", argv0, arg);
			usage(stderr, argv0);
			exit(2);
		}
	}

	childpid = 0;
	bool columnable = !longformat;
	if ( columnable && isatty(1) )
	{
		int pipes[2];
		if ( pipe(pipes) ) { perror("pipe"); return 1; }
		childpid = fork();
		if ( childpid < 0 ) { perror("fork"); return 1; }
		if ( childpid )
		{
			close(1);
			dup(pipes[1]);
			close(pipes[0]);
			close(pipes[1]);
		}
		else
		{
			close(0);
			dup(pipes[0]);
			close(pipes[0]);
			close(pipes[1]);
			const char* columner = "column";
			const char* argv[] = { columner, NULL };
			execvp(columner, (char* const*) argv);
			error(127, errno, "%s", columner);
		}
	}

	int result = 0;
	bool anyargs = false;
	for ( int i = 1; i < argc; i++ )
	{
		if ( !argv[i] ) { continue; }
		anyargs = true;
		if ( (result = ls(argv[i])) ) { break; }
	}

	if ( !anyargs ) { result = ls("."); }

	finishoutput();

	return result;
}

