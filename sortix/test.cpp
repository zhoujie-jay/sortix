#include "platform.h"
#include "globals.h"
#include "iprintable.h"
#include "log.h"

namespace Sortix
{
	void foo(int bar, unsigned int qux)
	{
		Log::PrintF("Hello, %i, and %u!", bar, qux);
	}
}
