/*******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011.

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

	uptime.cpp
	Tell how long the system has been running.

*******************************************************************************/

#include <stdio.h>
#include <unistd.h>

size_t Seconds(uintmax_t usecs)
{
	return (usecs / (1000ULL * 1000ULL)) % (60ULL);
}

size_t Minutes(uintmax_t usecs)
{
	return (usecs / (1000ULL * 1000ULL * 60ULL)) % (60ULL);
}

size_t Hours(uintmax_t usecs)
{
	return (usecs / (1000ULL * 1000ULL * 60ULL * 60ULL)) % (24ULL);
}

size_t Days(uintmax_t usecs)
{
	return usecs / (1000ULL * 1000ULL * 60ULL * 60ULL * 24ULL);
}

void PrintElement(size_t num, const char* single, const char* multiple)
{
	static const char* prefix = "";
	if ( !num ) { return; }
	const char* str = (num>1) ? multiple : single;
	printf("%s%zu %s", prefix, num, str);
	prefix = ", ";
}

int main(int argc, char* argv[])
{
	uintmax_t usecssinceboot;
	if ( uptime(&usecssinceboot) ) { perror("uptime"); return 1; }

	PrintElement(Days(usecssinceboot), "day", "days");
	PrintElement(Hours(usecssinceboot), "hour", "hours");
	PrintElement(Minutes(usecssinceboot), "min", "mins");
	PrintElement(Seconds(usecssinceboot), "sec", "secs");
	printf("\n");

	return 0;
}
