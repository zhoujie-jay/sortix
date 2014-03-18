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

    uname.cpp
    Print system information.

*******************************************************************************/

#include <sys/utsname.h>

#include <errno.h>
#include <error.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined(VERSIONSTR)
#define VERSIONSTR "unknown version"
#endif

const unsigned long PRINT_KERNELNAME = 1UL << 0UL;
const unsigned long PRINT_NODENAME = 1UL << 1UL;
const unsigned long PRINT_KERNELREL = 1UL << 2UL;
const unsigned long PRINT_KERNELVER = 1UL << 3UL;
const unsigned long PRINT_MACHINE = 1UL << 4UL;
const unsigned long PRINT_PROCESSOR = 1UL << 5UL;
const unsigned long PRINT_HWPLATFORM = 1UL << 6UL;
const unsigned long PRINT_OPSYS = 1UL << 7UL;

bool has_printed = false;

void DoPrint(const char* msg)
{
	if ( has_printed )
		printf(" ");
	printf("%s", msg);
	has_printed = true;
}

void Help(FILE* fp, const char* argv0)
{
	fprintf(fp, "Usage: %s [OPTION]...\n", argv0);
	fprintf(fp, "Print certain system information.\n");
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
	unsigned long flags = 0;
	for ( int i = 1; i < argc; i++ )
	{
		const char* arg = argv[i];
		if ( arg[0] != '-' )
		{
			fprintf(stderr, "%s: extra operand: %s\n", argv0, arg);
			Help(stderr, argv0);
			exit(1);
		}
		if ( arg[1] != '-' )
			for ( size_t i = 1; arg[i]; i++ )
				switch ( arg[i] )
				{
				case 'a': flags |= LONG_MAX; break;
				case 's': flags |= PRINT_KERNELNAME; break;
				case 'n': flags |= PRINT_NODENAME; break;
				case 'r': flags |= PRINT_KERNELREL; break;
				case 'v': flags |= PRINT_KERNELVER; break;
				case 'm': flags |= PRINT_MACHINE; break;
				case 'p': flags |= PRINT_PROCESSOR; break;
				case 'i': flags |= PRINT_HWPLATFORM; break;
				case 'o': flags |= PRINT_OPSYS; break;
				default:
					fprintf(stderr, "%s: unknown option -- '%c'\n", argv0, arg[i]);
					Help(stderr, argv0);
					exit(1);
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
		else if ( !strcmp(arg, "--help") ) { Help(stdout, argv0); exit(0); }
		else if ( !strcmp(arg, "--version") ) { Version(stdout, argv0); exit(0); }
		else
		{
			fprintf(stderr, "%s: unknown option: %s\n", argv0, arg);
			Help(stderr, argv0);
			exit(1);
		}
	}

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
