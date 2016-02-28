/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2014.

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

    chkblayout.c
    Changes the current keyboard layout.

*******************************************************************************/

#include <sys/stat.h>

#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <ioleast.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

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

static void help(FILE* fp, const char* argv0)
{
	fprintf(fp, "Usage: %s [OPTION...] [LAYOUT-NAME]\n", argv0);
	fprintf(fp, "Changes the current keyboard layout.\n");
	fprintf(fp, "\n");
	fprintf(fp, "Options supported by %s:\n", argv0);
	fprintf(fp, "  --help             Display this help and exit\n");
	fprintf(fp, "  --version          Output version information and exit\n");
}

static void version(FILE* fp, const char* argv0)
{
	fprintf(fp, "%s (Sortix) %s\n", argv0, VERSIONSTR);
	fprintf(fp, "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n");
	fprintf(fp, "This is free software: you are free to change and redistribute it.\n");
	fprintf(fp, "There is NO WARRANTY, to the extent permitted by law.\n");
}

int main(int argc, char* argv[])
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
			char c;
			while ( (c = *++arg) ) switch ( c )
			{
			default:
				fprintf(stderr, "%s: unknown option -- '%c'\n", argv0, c);
				help(stderr, argv0);
				exit(1);
			}
		}
		else if ( !strcmp(arg, "--help") )
			help(stdout, argv0), exit(0);
		else if ( !strcmp(arg, "--version") )
			version(stdout, argv0), exit(0);
		else
		{
			fprintf(stderr, "%s: unknown option: %s\n", argv0, arg);
			help(stderr, argv0);
			exit(1);
		}
	}

	compact_arguments(&argc, &argv);

	const char* tty_path = "/dev/tty";
	int tty_fd = open(tty_path, O_WRONLY);
	if ( tty_fd < 0 )
		error(1, errno, "`%s'", tty_path);
	if ( !isatty(tty_fd) )
		error(1, errno, "`%s'", tty_path);

	if ( argc == 1 )
		error(1, 0, "expected path to new keyboard layout");

	const char* kblayout_path = argv[1];
	if ( !strchr(kblayout_path, '/') )
	{
		char* new_kblayout_path;
		if ( asprintf(&new_kblayout_path, "/share/kblayout/%s", kblayout_path) < 0 )
			error(1, errno, "asprintf");
		kblayout_path = new_kblayout_path;
	}
	int kblayout_fd = open(kblayout_path, O_RDONLY);
	if ( kblayout_fd < 0 )
		error(1, errno, "`%s'", kblayout_path);

	struct stat kblayout_st;
	if ( fstat(kblayout_fd, &kblayout_st) < 0 )
		error(1, errno, "stat: `%s'", kblayout_path);

	if ( SIZE_MAX < (size_t) kblayout_st.st_size )
		error(1, EFBIG, "`%s'", kblayout_path);

	size_t kblayout_size = (size_t) kblayout_st.st_size;
	unsigned char* kblayout = (unsigned char*) malloc(kblayout_size);
	if ( !kblayout )
		error(1, errno, "malloc");
	if ( readall(kblayout_fd, kblayout, kblayout_size) != kblayout_size )
		error(1, errno, "read: `%s'", kblayout_path);
	close(kblayout_fd);

	if ( tcsetblob(tty_fd, "kblayout", kblayout, kblayout_size) < 0 )
		error(1, errno, "tcsetblob(\"kblayout\", `%s')", kblayout_path);

	free(kblayout);

	close(tty_fd);

	return 0;
}
