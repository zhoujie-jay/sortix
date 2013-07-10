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

    sys/kernelinfo.h
    Queries information about the kernel.

*******************************************************************************/

#ifndef _SYS_KERNELINFO_H
#define _SYS_KERNELINFO_H 1

#include <features.h>

__BEGIN_DECLS

@include(size_t.h)
@include(ssize_t.h)

ssize_t kernelinfo(const char* req, char* resp, size_t resplen);

__END_DECLS

#endif
