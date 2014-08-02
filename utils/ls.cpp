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
#include <locale.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#if !defined(VERSIONSTR)
#define VERSIONSTR "unknown version"
#endif

int current_year;

bool option_colors = false;
bool option_directory = false;
bool option_inode = false;
bool option_long_format = false;
bool option_show_dotdot = false;
bool option_show_dotfiles = false;
bool option_time_modified = false;

pid_t child_pid;

static struct dirent* dirent_dup(struct dirent* entry)
{
	struct dirent* copy = (struct dirent*) malloc(entry->d_reclen);
	if ( !copy )
		return NULL;
	memcpy(copy, entry, entry->d_reclen);
	return copy;
}

void finish_output()
{
	int errnum = errno;
	fflush(stdout);
	if ( child_pid )
	{
		int status;
		close(1);
		wait(&status);
		child_pid = 0;
	}
	errno = errnum;
}

void ls_error(int status, int errnum, const char* format, ...)
{
	finish_output();

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
	if ( option_time_modified &&
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

int handle_entry_internal(const char* fullpath, const char* name, unsigned char type)
{
	// TODO: Use openat and fstat.
	struct stat st;
	bool stat_error = false;
	if ( (option_long_format || type == DT_UNKNOWN || type == DT_REG) )
	{
		if ( stat(fullpath, &st) == 0 )
		{
			if ( S_ISBLK(st.st_mode) )
				type = DT_BLK;
			if ( S_ISCHR(st.st_mode) )
				type = DT_CHR;
			if ( S_ISDIR(st.st_mode) )
				type = DT_DIR;
			if ( S_ISFIFO(st.st_mode) )
				type = DT_FIFO;
			if ( S_ISLNK(st.st_mode) )
				type = DT_LNK;
			if ( S_ISREG(st.st_mode) )
				type = DT_REG;
			if ( S_ISSOCK(st.st_mode) )
				type = DT_SOCK;
		}
		else
		{
			memset(&st, 0, sizeof(st));
			stat_error = true;
		}
	}

	const char* color_pre = "";
	const char* color_post = "";
	if ( option_colors )
	{
		color_post = "\e[m";
		switch ( type )
		{
		case DT_UNKNOWN: color_pre = "\e[91m"; break;
		case DT_BLK: color_pre = "\e[93m"; break;
		case DT_CHR: color_pre = "\e[93m"; break;
		case DT_DIR: color_pre = "\e[36m"; break;
		case DT_FIFO: color_pre = "\e[33m"; break;
		case DT_LNK: color_pre = "\e[96m"; break;
		case DT_SOCK: color_pre = "\e[35m"; break;
		case DT_REG:
			if ( type == DT_REG && !stat_error && (st.st_mode & 0111) )
				color_pre = "\e[32m";
			else
				color_post = "";
			break;
		default:
			color_post = "";
		}
	}

	if ( option_inode )
		printf("%ju ", (uintmax_t) st.st_ino);
	if ( !option_long_format )
	{
		printf("%s%s%s\n", color_pre, name, color_post);
		return 0;
	}
	char perms[11];
	perms[0] = '?';
	switch ( type )
	{
		case DT_UNKNOWN: perms[0] = '?'; break;
		case DT_BLK: perms[0] = 'b'; break;
		case DT_CHR: perms[0] = 'c'; break;
		case DT_DIR: perms[0] = 'd'; break;
		case DT_FIFO: perms[0] = 'p'; break;
		case DT_LNK: perms[0] = 'l'; break;
		case DT_REG: perms[0] = '-'; break;
		case DT_SOCK: perms[0] = 's'; break;
		default: perms[0] = '?'; break;
	}
	const char flagnames[] = { 'x', 'w', 'r' };
	for ( size_t i = 0; i < 9; i++ )
	{
		bool set = st.st_mode & (1UL<<i);
		perms[9-i] = stat_error ? '?' : set ? flagnames[i % 3] : '-';
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
		color_pre, name, color_post);
	return 0;
}

int handle_entry(const char* path, const char* name, unsigned char type)
{
	bool isdotdot = strcmp(name, ".") == 0 || strcmp(name, "..") == 0;
	bool isdotfile = !isdotdot && name[0] == '.';
	if ( isdotdot && !option_show_dotdot && !option_directory )
		return 0;
	if ( isdotfile && !option_show_dotfiles && !option_directory )
		return 0;
	char* fullpath;
	asprintf(&fullpath, "%s/%s", path, name);
	int result = handle_entry_internal(fullpath, name, type);
	free(fullpath);
	return result;
}

int ls(const char* path)
{
	if ( option_directory )
		return handle_entry_internal(path, path, DT_UNKNOWN);

	int ret = 1;
	DIR* dir;
	const size_t DEFAULT_ENTRIES_LEN = 4UL;
	size_t entrieslen = DEFAULT_ENTRIES_LEN;
	size_t entriesused = 0;
	size_t entriessize = sizeof(struct dirent*) * entrieslen;
	struct dirent** entries = (struct dirent**) malloc(entriessize);
	if ( !entries )
	{
		ls_error(0, errno, "malloc");
		goto cleanup_done;
	}

	dir = opendir(path);
	if ( !dir )
	{
		if ( errno == ENOTDIR )
			return handle_entry_internal(path, path, DT_UNKNOWN);
		perror(path);
		ret = 2;
		goto cleanup_entries;
	}

	while ( struct dirent* entry = readdir(dir) )
	{
		if ( entriesused == entrieslen )
		{
			size_t newentrieslen = entrieslen * 2UL;
			struct dirent** newentries;
			size_t newentriessize = sizeof(struct dirent*) * newentrieslen;
			newentries = (struct dirent**) realloc(entries, newentriessize);
			if ( !newentries )
			{
				ls_error(0, errno, "realloc");
				goto cleanup_dir;
			}
			entries = newentries;
			entrieslen = newentrieslen;
			entriessize = newentriessize;
		}
		struct dirent* copy = dirent_dup(entry);
		if ( !copy )
		{
			ls_error(0, errno, "malloc");
			goto cleanup_dir;
		}
		entries[entriesused++] = copy;
	}

#if defined(sortix)
	if ( derror(dir) )
	{
		ls_error(0, errno, path);
		goto cleanup_dir;
	}
#endif

	sort_dirents_dirfd = dirfd(dir);
	qsort(entries, entriesused, sizeof(*entries), sort_dirents);

	for ( size_t i = 0; i < entriesused; i++ )
	{
		if ( handle_entry(path, entries[i]->d_name, entries[i]->d_type) != 0 )
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

static void compact_arguments(int* argc, char*** argv)
{
	for ( int i = 0; i < *argc; i++ )
	{
		while ( i < *argc && !(*argv)[i] )
		{
			for ( int n = i; n < *argc; n++ )
				(*argv)[n] = (*argv)[n+1];
			(*argc)--;
		}
	}
}

static void version(FILE* fp, const char* argv0)
{
	fprintf(fp, "%s (Sortix) %s\n", argv0, VERSIONSTR);
	fprintf(fp, "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n");
	fprintf(fp, "This is free software: you are free to change and redistribute it.\n");
	fprintf(fp, "There is NO WARRANTY, to the extent permitted by law.\n");
}

static void help(FILE* fp, const char* argv0)
{
	fprintf(fp, "Usage: %s [OPTION]... [FILE]...\n", argv0);
}

int main(int argc, char** argv)
{
	setlocale(LC_ALL, "");

	const char* argv0 = argv[0];
	for ( int i = 1; i < argc; i++ )
	{
		const char* arg = argv[i];
		if ( arg[0] != '-' || !arg[1] )
			continue;
		argv[i] = NULL;
		if ( !strcmp(arg, "--") )
			break;
		if ( arg[1] != '-' )
		{
			while ( char c = *++arg ) switch ( c )
			{
			case 'a': option_show_dotdot = true, option_show_dotfiles = true; break;
			case 'A': option_show_dotdot = false, option_show_dotfiles = true; break;
			case 'd': option_directory = true; break;
			case 'i': option_inode = true; break;
			case 'l': option_long_format = true; break;
			case 't': option_time_modified = true; break;
			default:
				fprintf(stderr, "%s: unknown option -- '%c'\n", argv0, c);
				help(stderr, argv0);
				exit(2);
			}
		}
		else if ( !strcmp(arg, "--version") )
			version(stdout, argv0), exit(0);
		else if ( !strcmp(arg, "--help") )
			help(stdout, argv0), exit(0);
		else if ( !strcmp(arg, "--all") )
			option_show_dotdot = false, option_show_dotfiles = true;
		else if ( !strcmp(arg, "--almost-all") )
			option_show_dotdot = false, option_show_dotfiles = true;
		else if ( !strcmp(arg, "--directory") )
			option_directory = true;
		else if ( !strcmp(arg, "--inode") )
			option_inode = true;
		else
		{
			fprintf(stderr, "%s: unrecognized option: %s\n", argv0, arg);
			help(stderr, argv0);
			exit(2);
		}
	}

	compact_arguments(&argc, &argv);

	time_t current_time;
	struct tm current_year_tm;
	time(&current_time);
	localtime_r(&current_time, &current_year_tm);
	current_year = current_year_tm.tm_year;

	if ( isatty(1) )
		option_colors = true;

	child_pid = 0;
	bool columnable = !option_long_format;
	if ( columnable && isatty(1) )
	{
		int pipes[2];
		if ( pipe(pipes) )
			error(1, errno, "pipe");
		child_pid = fork();
		if ( child_pid < 0 )
			error(1, errno, "fork");
		if ( child_pid )
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

	if ( 2 <= argc )
	{
		// TODO: This isn't the strictly correct semantics:
		if ( option_time_modified )
			qsort(argv + 1, argc - 1, sizeof(char*), argv_mtim_compare);

		for ( int i = 1; i < argc; i++ )
			if ( (result = ls(argv[i])) != 0 )
				break;
	}
	else
	{
		result = ls(".");
	}

	finish_output();

	return result;
}
