/*******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011, 2012.

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

	help.cpp
	Prints a friendly message and ls /bin.

*******************************************************************************/

#include <stdio.h>
#include <errno.h>
#include <error.h>
#include <unistd.h>

int main(int /*argc*/, char* /*argv*/[])
{
	printf("Please enter the name of one of the following programs:\n");

	const char* programname = "ls";
	const char* newargv[] = { programname, "/bin", NULL };
	execvp(programname, (char* const*) newargv);
	error(1, errno, "%s", programname);

	return 1;
}
