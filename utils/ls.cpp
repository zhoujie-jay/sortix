#include <stdio.h>
#include <libmaxsi/platform.h>
#include <libmaxsi/process.h>

using namespace Maxsi;

int main(int argc, char* argv[])
{
	size_t numfiles = Process::GetNumFiles();
	for ( size_t i = 0; i < numfiles; i++ )
	{
		FileInfo fileinfo;
		Process::GetFileInfo(i, &fileinfo);
		printf("%s\n", fileinfo.name);
	}

	return 0;
}
