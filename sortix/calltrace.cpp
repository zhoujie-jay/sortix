/*******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2012.

	This file is part of Sortix.

	Sortix is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License as published by the Free Software
	Foundation, either version 3 of the License, or (at your option) any later
	version.

	Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
	details.

	You should have received a copy of the GNU General Public License along with
	Sortix. If not, see <http://www.gnu.org/licenses/>.

	calltrace.cpp
	Traverses the stack and prints the callstack, which aids debugging.

*******************************************************************************/

#include <sortix/kernel/platform.h>
#include "calltrace.h"

namespace Sortix {
namespace Calltrace {

extern "C" void calltrace();
extern "C" void calltrace_print_function(size_t index, addr_t ip)
{
	Log::PrintF("%zu: 0x%zx\n", index, ip);
}

void Perform()
{
	Log::PrintF("Calltrace:\n");
	calltrace();
}

} // namespace Calltrace
} // namespace Sortix
