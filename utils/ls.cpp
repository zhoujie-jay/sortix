/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2014.

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

#include <dirent.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

int current_year;

#if !defined(VERSIONSTR)
#define VERSIONSTR "unknown version"
#endif

bool directory = false;
bool inode = false;
bool longformat = false;
bool showdotdot = false;
bool showdotfiles = false;
bool time_modified = false;
bool colors = false;

pid_t childpid;

void finishoutput()
{
	int errnum = errno;
	int status;
	fflush(stdout);
	if ( childpid ) { close(1); wait(&status); }
	childpid = 0;
	errno = errnum;
}

void ls_perror(const char* s)
{
	finishoutput();
	perror(s);
}

static struct dirent* dirent_dup(struct dirent* entry)
{
	struct dirent* copy = (struct dirent*) malloc(entry->d_reclen);
	if ( !copy )
		return NULL;
	memcpy(copy, entry, entry->d_reclen);
	return copy;
}

void ls_error(int status, int errnum, const char* format, ...)
{
	finishoutput();

	// TODO: The rest is just plain generic gnu_error(3), how about a function
	// called gnu_verror(3) that accepts a va_list.
	fprintf(stderr, "%s: ", program_invocation_name);

	va_list list;
	va_start(list, format);
	vfprintf(stderr, format, list);
	va_end(list);

	if ( errnum )
		fprintf(stderr, ": %s", strerror(errnum));
	fprintf(stderr, "\n");
	if ( status )
		exit(status);
}

int argv_mtim_compare(const void* a_void, const void* b_void)
{
	const char* a = *(const char**) a_void;
	const char* b = *(const char**) b_void;

	if ( !a && b )
		return 1;
	if ( a && !b )
		return -1;
	if ( !a && !b )
		return 0;

	struct stat a_st;
	struct stat b_st;
	if ( stat(a, &a_st) == 0 && stat(b, &b_st) == 0 )
	{
		if ( a_st.st_mtim.tv_sec < b_st.st_mtim.tv_sec )
			return 1;
		if ( a_st.st_mtim.tv_sec > b_st.st_mtim.tv_sec )
			return -1;
		if ( a_st.st_mtim.tv_nsec < b_st.st_mtim.tv_nsec )
			return 1;
		if ( a_st.st_mtim.tv_nsec > b_st.st_mtim.tv_nsec )
			return -1;
		return 0;
	}
	return strcmp(a, b);
}

int sort_dirents_dirfd;

int sort_dirents(const void* a_void, const void* b_void)
{
	struct dirent* a = *(struct dirent**) a_void;
	struct dirent* b = *(struct dirent**) b_void;
	struct stat a_st;
	struct stat b_st;
	if ( time_modified &&
	     fstatat(sort_dirents_dirfd, a->d_name, &a_st, 0) == 0 &&
	     fstatat(sort_dirents_dirfd, b->d_name, &b_st, 0) == 0 )
	{
		if ( a_st.st_mtim.tv_sec < b_st.st_mtim.tv_sec )
			return 1;
		if ( a_st.st_mtim.tv_sec > b_st.st_mtim.tv_sec )
			return -1;
		if ( a_st.st_mtim.tv_nsec < b_st.st_mtim.tv_nsec )
			return 1;
		if ( a_st.st_mtim.tv_nsec > b_st.st_mtim.tv_nsec )
			return -1;
		return 0;
	}
	return strcmp(a->d_name, b->d_name);
}

void getentrycolor(const char** pre, const char** post, mode_t mode)
{
	*pre = "";
	*post = "";
	if ( !colors )
		return;
	if ( S_ISDIR(mode) )
		*pre = "\e[36m",
		*post = "\e[37m";
	else if ( mode & 0111 )
		*pre = "\e[32m",
		*post = "\e[37m";
}

int handleentry_internal(const char* fullpath, const char* name)
{
	// TODO: Use openat and fstat.
	struct stat st;
	if ( stat(fullpath, &st) )
	{
		finishoutput();
		error(0, errno, "stat: %s", fullpath);
		return 2;
	}
	const char* colorpre;
	const char* colorpost;
	getentrycolor(&colorpre, &colorpost, st.st_mode);
	if ( inode )
		printf("%ju ", (uintmax_t) st.st_ino);
	if ( !longformat )
	{
		printf("%s%s%s\n", colorpre, name, colorpost);
		return 0;
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
	struct tm mod_tm;
	localtime_r(&st.st_mtim.tv_sec, &mod_tm);
	char time_str[64];
	if ( current_year == mod_tm.tm_year )
		strftime(time_str, 64, "%b %e %H:%M", &mod_tm);
	else
		strftime(time_str, 64, "%b %e  %Y", &mod_tm);
	printf("%s %3ju root root %4ju%s %s %s%s%s\n",
		perms,
		(uintmax_t) st.st_nlink,
		(uintmax_t) size, sizeunit,
		time_str,
		colorpre, name, colorpost);
	return 0;
}

int handleentry(const char* path, const char* name)
{
	bool isdotdot = strcmp(name, ".") == 0 || strcmp(name, "..") == 0;
	bool isdotfile = !isdotdot && name[0] == '.';
	if ( isdotdot && !showdotdot && !directory ) { return 0; }
	if ( isdotfile && !showdotfiles && !directory ) { return 0; }
	char* fullpath = new char[strlen(path) + 1 + strlen(name) + 1];
	strcpy(fullpath, path);
	strcat(fullpath, "/");
	strcat(fullpath, name);
	int result = handleentry_internal(fullpath, name);
	delete[] fullpath;
	return result;
}

int ls(const char* path)
{
	time_t current_time;
	struct tm current_year_tm;
	time(&current_time);
	localtime_r(&current_time, &current_year_tm);
	current_year = current_year_tm.tm_year;

	if ( directory )
		return handleentry_internal(path, path);

	int ret = 1;
	DIR* dir;
	const size_t DEFAULT_ENTRIES_LEN = 4UL;
	size_t entrieslen = DEFAULT_ENTRIES_LEN;
	size_t entriesused = 0;
	size_t entriessize = sizeof(struct dirent*) * entrieslen;
	struct dirent** entries = (struct dirent**) malloc(entriessize);
	if ( !entries ) { ls_perror("malloc"); goto cleanup_done; }

	dir = opendir(path);
	if ( !dir )
	{
		if ( errno == ENOTDIR )
			return handleentry_internal(path, path);
		perror(path);
		ret = 2;
		goto cleanup_entries;
	}

	struct dirent* entry;
	while ( (entry = readdir(dir)) )
	{
		if ( entriesused == entrieslen )
		{
			size_t newentrieslen = entrieslen * 2UL;
			struct dirent** newentries;
			size_t newentriessize = sizeof(struct dirent*) * newentrieslen;
			newentries = (struct dirent**) realloc(entries, newentriessize);
			if ( !newentries ) { ls_perror("realloc"); goto cleanup_dir; }
			entries = newentries;
			entrieslen = newentrieslen;
			entriessize = newentriessize;
		}
		struct dirent* copy = dirent_dup(entry);
		if ( !copy ) { ls_perror("malloc"); goto cleanup_dir; }
		entries[entriesused++] = copy;
	}

#if defined(sortix)
	if ( derror(dir) ) { perror(path); goto cleanup_dir; }
#endif

	sort_dirents_dirfd = dirfd(dir);
	qsort(entries, entriesused, sizeof(*entries), sort_dirents);

	for ( size_t i = 0; i < entriesused; i++ )
	{
		if ( handleentry(path, entries[i]->d_name) != 0 )
			goto cleanup_dir;
	}

	ret = 0;

cleanup_dir:
	closedir(dir);
cleanup_entries:
	for ( size_t i = 0; i < entriesused; i++ )
		free(entries[i]);
	free(entries);
cleanup_done:
	return ret;
}

void version(FILE* fp, const char* argv0)
{
	fprintf(fp, "%s (Sortix) %s\n", argv0, VERSIONSTR);
	fprintf(fp, "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n");
	fprintf(fp, "This is free software: you are free to change and redistribute it.\n");
	fprintf(fp, "There is NO WARRANTY, to the extent permitted by law.\n");
}

void help(FILE* fp, const char* argv0)
{
	fprintf(fp, "usage: %s [-l] [-a | -A] [<DIR> ...]\n", argv0);
}

int main(int argc, char** argv)
{
	const char* argv0 = argv[0];
	for ( int i = 1; i < argc; i++ )
	{
		const char* arg = argv[i];
		if ( arg[0] != '-' || !arg[1] ) { continue; }
		argv[i] = NULL;
		if ( !strcmp(arg, "--") ) { break; }
		if ( arg[1] != '-' )
		{
			char c;
			while ( (c = *++arg) )
			{
				switch ( c )
				{
				case 'd':
					directory = true;
					break;
				case 'i':
					inode = true;
					break;
				case 'l':
					longformat = true;
					break;
				case 't':
					time_modified = true;
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
					help(stderr, argv0);
					exit(2);
				}
			}
		}
		else if ( !strcmp(arg, "--version") )
		{
			version(stdout, argv0); exit(0);
		}
		else if ( !strcmp(arg, "--help") )
		{
			help(stdout, argv0); exit(0);
		}
		else
		{
			fprintf(stderr, "%s: unrecognized option: %s\n", argv0, arg);
			help(stderr, argv0);
			exit(2);
		}
	}

	if ( isatty(1) )
		colors = true;

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

	// TODO: This isn't the strictly correct semantics:
	if ( time_modified )
		qsort(argv + 1, argc - 1, sizeof(char*), argv_mtim_compare);

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
