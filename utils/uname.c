/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2014.

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

    uname.c
    Print system information.

*******************************************************************************/

#include <sys/utsname.h>

#include <errno.h>
#include <error.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const int PRINT_KERNELNAME = 1 << 0;
static const int PRINT_NODENAME = 1 << 1;
static const int PRINT_KERNELREL = 1 << 2;
static const int PRINT_KERNELVER = 1 << 3;
static const int PRINT_MACHINE = 1 << 5;
static const int PRINT_PROCESSOR = 1 << 6;
static const int PRINT_HWPLATFORM = 1 << 7;
static const int PRINT_OPSYS = 1 << 8;

bool has_printed = false;

void DoPrint(const char* msg)
{
	if ( has_printed )
		printf(" ");
	printf("%s", msg);
	has_printed = true;
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
	fprintf(fp, "Usage: %s [OPTION]...\n", argv0);
	fprintf(fp, "Print certain system information.\n");
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
	const char* argv0 = argv[0];
	int flags = 0;
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
			case 'a': flags |= INT_MAX; break;
			case 's': flags |= PRINT_KERNELNAME; break;
			case 'n': flags |= PRINT_NODENAME; break;
			case 'r': flags |= PRINT_KERNELREL; break;
			case 'v': flags |= PRINT_KERNELVER; break;
			case 'm': flags |= PRINT_MACHINE; break;
			case 'p': flags |= PRINT_PROCESSOR; break;
			case 'i': flags |= PRINT_HWPLATFORM; break;
			case 'o': flags |= PRINT_OPSYS; break;
			default:
				fprintf(stderr, "%s: unknown option -- '%c'\n", argv0, c);
				help(stderr, argv0);
				exit(1);
			}
		}
		else if ( !strcmp(arg, "--kernel-name") )
			flags |= PRINT_KERNELNAME;
		else if ( !strcmp(arg, "--nodename") )
			flags |= PRINT_NODENAME;
		else if ( !strcmp(arg, "--kernel-release") )
			flags |= PRINT_KERNELREL;
		else if ( !strcmp(arg, "--kernel-version") )
			flags |= PRINT_KERNELVER;
		else if ( !strcmp(arg, "--machine") )
			flags |= PRINT_MACHINE;
		else if ( !strcmp(arg, "--processor") )
			flags |= PRINT_PROCESSOR;
		else if ( !strcmp(arg, "--hardware-platform") )
			flags |= PRINT_HWPLATFORM;
		else if ( !strcmp(arg, "--operating-system") )
			flags |= PRINT_OPSYS;
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

	if ( 1 < argc )
		error(1, 0, "extra operand");

	static struct utsname utsname;
	if ( uname(&utsname) < 0 )
		error(1, errno, "uname");

	if ( !flags )
		flags = PRINT_KERNELNAME;
	if ( flags & PRINT_KERNELNAME )
		DoPrint(utsname.sysname);
	if ( flags & PRINT_NODENAME )
		DoPrint(utsname.nodename);
	if ( flags & PRINT_KERNELREL )
		DoPrint(utsname.release);
	if ( flags & PRINT_KERNELVER )
		DoPrint(utsname.version);
	if ( flags & PRINT_MACHINE )
		DoPrint(utsname.machine);
	if ( flags & PRINT_PROCESSOR )
		DoPrint(utsname.processor);
	if ( flags & PRINT_HWPLATFORM )
		DoPrint(utsname.hwplatform);
	if ( flags & PRINT_OPSYS )
		DoPrint(utsname.opsysname);
	printf("\n");
	return 0;
}
