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

    test-fmemopen.c
    Tests basic fmemopen() usage.

*******************************************************************************/

#include <stdio.h>
#include <string.h>

#include "test.h"

int main(void)
{
	const char* string = "VYl/plYoFGAg2";
	size_t string_length = strlen(string);

	FILE* fp;
	int c;

	unsigned char* buffer = (unsigned char*) string;
	size_t buffer_size = string_length;

	if ( !(fp = fmemopen(buffer, buffer_size, "r")) )
		test_error(errno, "fmemopen");

	for ( size_t i = 0; i < buffer_size; i++ )
	{
		c = fgetc(fp);
		test_assert(!(c == EOF && feof(fp)));
		test_assert(!(c == EOF && ferror(fp)));
		test_assert(c != EOF);
		test_assert((unsigned char) c == buffer[i]);
		test_assert(!feof(fp));
		test_assert(!ferror(fp));
	}

	c = fgetc(fp);
	test_assert(c == EOF);
	test_assert(!ferror(fp));
	test_assert(feof(fp));

	fclose(fp);

	return 0;
}
