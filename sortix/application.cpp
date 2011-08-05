/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011.

	This file is part of Sortix.

	Sortix is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License as published by the Free Software
	Foundation, either version 3 of the License, or (at your option) any later
	version.

	Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
	details.

	You should have received a copy of the GNU General Public License along
	with Sortix. If not, see <http://www.gnu.org/licenses/>.

	application.cpp
	A test application for the scheduler and libmaxsi.

******************************************************************************/

#include <libmaxsi/platform.h>
#include <libmaxsi/thread.h>
#include <libmaxsi/io.h>

using namespace Maxsi;

void* TestThread(void* Parameter)
{
	for ( size_t I = 0; I < 64; I++ )
	{
		StdOut::Print(".");
		Thread::USleep(20*1000); // Sleep 100 * 1000 microseconds = 100 ms.
	}

	StdOut::Print("\n");

	return NULL;
}

// The entry point in this sample program for sortix.
void* RunApplication(void* Parameter)
{
	StdOut::Print("Hello, says the thread!\n");

	const char* Message = "Oooh! And message passing works!\n";

	while ( true )
	{
		StdOut::Print("Created Thread: ");
		if ( Thread::Create(TestThread, Message) == 0 )
		{
			StdOut::Print("ERROR: Could not create child thread\n");
		}
		Thread::USleep(1300 * 1000); // Sleep 2 seconds.
	}

	return NULL;
}

