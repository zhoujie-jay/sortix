/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012.

    This file is part of dispd.

    dispd is free software: you can redistribute it and/or modify it under the
    terms of the GNU Lesser General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    dispd is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
    details.

    You should have received a copy of the GNU Lesser General Public License
    along with dispd. If not, see <http://www.gnu.org/licenses/>.

    dispd.cpp
    Main entry point of the Sortix Display Daemon.

*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>

int main(int /*argc*/, char* /*argv*/[])
{
	fprintf(stderr, "dispd is not implemented as a server yet.\n");
	exit(1);
}
