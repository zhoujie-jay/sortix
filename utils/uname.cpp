#include <sys/kernelinfo.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <error.h>

int main(int argc, char* argv[])
{
	size_t VERSIONSIZE = 256UL;
	char version[VERSIONSIZE];
	if ( kernelinfo("version", version, VERSIONSIZE) )
	{
		error(1, errno, "kernelinfo(\"version\")");
	}
	printf("Sortix %zu bit (%s)\n", sizeof(size_t) * 8UL, version);
	return 0;
}

