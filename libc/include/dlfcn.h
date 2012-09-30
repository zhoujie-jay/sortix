/*******************************************************************************

	Copyright(C) Jonas 'Sortie' Termansen 2012.

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

	dlfcn.h
	Dynamic linking.

*******************************************************************************/

#ifndef	_DLFCN_H
#define	_DLFCN_H 1

#include <features.h>

__BEGIN_DECLS

#define RTLD_LAZY (1<<0)
#define RTLD_NOW (1<<1)
#define RTLD_GLOBAL (1<<8)
#define RTLD_LOCAL 0 /* Bit 8 is not set. */

int dlclose(void* handle);
char* dlerror(void* handle);
void* dlopen(const char* filename, int mode);
void* dlsym(void* handle, const char* name);

__END_DECLS

#endif