/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2015.

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

    scram.h
    Emergency process shutdown.

*******************************************************************************/

#ifndef _SCRAM_H
#define _SCRAM_H

#include <sys/cdefs.h>

#define SCRAM_ASSERT 1
#define SCRAM_STACK_SMASH 2
#define SCRAM_UNDEFINED_BEHAVIOR 3

struct scram_assert
{
	const char* filename;
	unsigned long line;
	const char* function;
	const char* expression;
};

struct scram_undefined_behavior
{
	const char* filename;
	unsigned long line;
	unsigned long column;
	const char* violation;
};

#ifdef __cplusplus
extern "C" {
#endif

__attribute__((noreturn))
void scram(int, const void*);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
