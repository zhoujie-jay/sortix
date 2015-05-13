/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

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

    getopt.h
    Command-line parsing facility.

*******************************************************************************/

#ifndef INCLUDE_GETOPT_H
#define INCLUDE_GETOPT_H

#include <sys/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

/* These declarations are repeated in <unistd.h>. */
#ifndef __getopt_unistd_shared_declared
#define __getopt_unistd_shared_declared
extern char* optarg;
extern int opterr;
extern int optind;
extern int optopt;

int getopt(int, char* const*, const char*);
#endif

struct option
{
	const char* name;
	int has_arg;
	int* flag;
	int val;
};

#define no_argument 0
#define required_argument 1
#define optional_argument 2

int getopt_long(int, char* const*, const char*, const struct option*, int*);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
