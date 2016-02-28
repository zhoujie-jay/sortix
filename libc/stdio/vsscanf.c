/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012, 2014.

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

    stdio/vsscanf.c
    Input format conversion.

*******************************************************************************/

#include <assert.h>
#include <stdio.h>

struct vsscanf_input
{
	union
	{
		const char* str;
		const unsigned char* ustr;
	};
	size_t offset;
};

static int vsscanf_fgetc(void* fp)
{
	struct vsscanf_input* input = (struct vsscanf_input*) fp;
	if ( !input->ustr[input->offset] )
		return EOF;
	return (int) input->ustr[input->offset++];
}

static int vsscanf_ungetc(int c, void* fp)
{
	struct vsscanf_input* input = (struct vsscanf_input*) fp;
	if ( c == EOF && !input->ustr[input->offset] )
		return c;
	assert(input->offset);
	input->offset--;
	assert(input->ustr[input->offset] == (unsigned char) c);
	return c;
}

int vsscanf(const char* str, const char* format, va_list ap)
{
	struct vsscanf_input input;
	input.str = str;
	input.offset = 0;
	return vscanf_callback(&input, vsscanf_fgetc, vsscanf_ungetc, format, ap);
}
