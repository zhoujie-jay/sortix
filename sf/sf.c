/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2015.

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

    sf.c
    Transmit and receive frames over serial connections.

*******************************************************************************/

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static bool try_read(int fd, const char* fd_path, unsigned char* c)
{
	ssize_t amount = read(fd, c, 1);
	if ( amount < 0 )
		err(1, "read: %s", fd_path);
	return amount == 1;
}

static void try_write(int fd, const char* fd_path, unsigned char c)
{
	ssize_t amount = write(fd, &c, 1);
	if ( amount < 0 )
		err(1, "write: %s", fd_path);
	if ( amount == 0 )
		errx(1, "write: %s: End of file condition", fd_path);
}

static void receive(int fd, const char* fd_path)
{
	unsigned char c;
	while ( true )
	{
		if ( !try_read(fd, fd_path, &c) )
			return;
		if ( c != 0xF7 )
			continue;
		if ( !try_read(fd, fd_path, &c) )
			return;
		if ( c == 0xFF )
			break;
	}
	while ( true )
	{
		if ( !try_read(fd, fd_path, &c) )
			return;
		if ( c == 0xF7 )
		{
			if ( !try_read(fd, fd_path, &c) )
				return;
			if ( c == 0xFE )
				break;
			if ( c == 0xFD )
				try_write(1, "stdout", 0xF7);
			continue;
		}
		try_write(1, "stdout", c);
	}
}

static void transmit(int fd, const char* fd_path)
{
	try_write(fd, fd_path, 0xF7);
	try_write(fd, fd_path, 0xFF);
	unsigned char c;
	while ( try_read(0, "stdin", &c) )
	{
		if ( c == 0xF7 )
		{
			try_write(fd, fd_path, 0xF7);
			try_write(fd, fd_path, 0xFD);
			continue;
		}
		try_write(fd, fd_path, c);
	}
	try_write(fd, fd_path, 0xF7);
	try_write(fd, fd_path, 0xFE);
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

static void help(FILE* fp, const char* argv0)
{
	fprintf(fp, "Usage: %s [OPTION...] [DEVICE]\n", argv0);
	fprintf(fp, "Transmit and receive frames over serial connections.\n");
	fprintf(fp, "\n");
	fprintf(fp, "Mandatory arguments to long options are mandatory for short options too.\n");
	fprintf(fp, "  -i, --input               receive a frame\n");
	fprintf(fp, "  -t, --output              transmit a frame\n");
	fprintf(fp, "      --help                display this help and exit\n");
	fprintf(fp, "      --version             output version information and exit\n");
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

	bool flag_input = false;
	bool flag_output = false;

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
			case 'i': flag_input = true; break;
			case 'o': flag_output = true; break;
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
		else if ( !strcmp(arg, "--input") )
			flag_input = true;
		else if ( !strcmp(arg, "--output") )
			flag_output = true;
		else
		{
			fprintf(stderr, "%s: unknown option: %s\n", argv0, arg);
			help(stderr, argv0);
			exit(1);
		}
	}

	compact_arguments(&argc, &argv);

	if ( flag_input + flag_output == 0 )
		errx(1, "need to specify exactly one of the -io options");

	if ( flag_input + flag_output > 1 )
		errx(1, "specified multiple of the incompatible -io options");

	if ( 3 <= argc )
		errx(1, "extra operand `%s'", argv[2]);

	int fd = flag_input ? 0 : 1;
	const char* fd_path = flag_input ? "stdin" : "stdout";

	if ( 2 <= argc )
	{
		fd_path = argv[1];
		struct stat st;
		if ( stat(fd_path, &st) == 0 && S_ISSOCK(st.st_mode) )
		{
			if ( (fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0 )
				err(1, "socket");
			struct sockaddr_un* addr;
			size_t addr_size = sizeof(*addr) - sizeof(addr->sun_path) +
			                   strlen(fd_path) + 1;
			if ( addr_size < sizeof(*addr) )
				addr_size = sizeof(*addr);
			if ( !(addr = (struct sockaddr_un*) malloc(addr_size)) )
				err(1, "malloc");
			memset(addr, 0, addr_size);
			addr->sun_family = AF_UNIX;
			strcpy(addr->sun_path, fd_path);
			if ( connect(fd, (const struct sockaddr*) addr, addr_size) < 0 )
				err(1, "connect: %s", fd_path);
		}
		else if ( (fd = open(fd_path, flag_input ? O_RDONLY : O_WRONLY)) < 0 )
			err(1, "%s", fd_path);
	}

	(flag_input ? receive : transmit)(fd, fd_path);

	if ( fd != 0 && fd != 1 )
		close(fd);

	return 0;
}
