/*******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2012.

	This file is part of LibMaxsi.

	LibMaxsi is free software: you can redistribute it and/or modify it under
	the terms of the GNU Lesser General Public License as published by the Free
	Software Foundation, either version 3 of the License, or (at your option)
	any later version.

	LibMaxsi is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
	details.

	You should have received a copy of the GNU Lesser General Public License
	along with LibMaxsi. If not, see <http://www.gnu.org/licenses/>.

	dlfcn.cpp
	Dynamic linking.

*******************************************************************************/

#include <stdio.h>
#include <dlfcn.h>

static const char* dlerrormsg = NULL;

extern "C" void* dlopen(const char* filename, int mode)
{
	dlerrormsg = "Sortix does not yet support dynamic linking";
	fprintf(stderr, "%s: loading file: %s\n", dlerrormsg, filename);
	return NULL;
}

extern "C" void* dlsym(void* handle, const char* name)
{
	dlerrormsg = "Sortix does not yet support dynamic linking";
	fprintf(stderr, "%s: resolving symbol: %s\n", dlerrormsg, name);
	return NULL;
}

extern "C" int dlclose(void* handle)
{
	return 0;
}

extern "C" char* dlerror(void* handle)
{
	const char* result = dlerrormsg;
	dlerrormsg = NULL;
	return (char*) dlerrormsg;
}
