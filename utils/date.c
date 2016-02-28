/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

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

    date.c
    Print or set system date and time.

*******************************************************************************/

#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

int main(void)
{
	time_t current_time = time(NULL);

	struct tm tm;
	if ( !localtime_r(&current_time, &tm) )
		error(1, errno, "time(%ji)", (intmax_t) current_time);

	const size_t BUFFER_SIZE = 256;
	char buffer[BUFFER_SIZE];
	strftime(buffer, BUFFER_SIZE, "%a %b %d %H:%M:%S %Y", &tm);
	printf("%s\n", buffer);

	return 0;
}
