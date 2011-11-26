#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <error.h>
#include <sys/readdirents.h>
#include <libmaxsi/platform.h>
#include <libmaxsi/process.h>

using namespace Maxsi;

int ls(const char* path)
{
	int fd = open(path, O_SEARCH | O_DIRECTORY);
	if ( fd < 0 ) { error(2, errno, "%s", path); return 2; }

	const size_t BUFFER_SIZE = 512;
	char buffer[BUFFER_SIZE];
	sortix_dirent* dirent = (sortix_dirent*) buffer;

	// TODO: Hack until mountpoints work correctly.
	if ( strcmp(path, "/") == 0 ) { printf("bin\ndev\n"); }

	while ( true )
	{
		if ( readdirents(fd, dirent, BUFFER_SIZE) )
		{
			error(2, errno, "readdirents: %s", path);
			return 1;
		}

		for ( sortix_dirent* iter = dirent; iter; iter = iter->d_next )
		{
			if ( iter->d_namelen == 0 ) { return 0; }
			printf("%s\n", iter->d_name);
		}
	}
}

int main(int argc, char* argv[])
{
	const size_t CWD_SIZE = 512;
	char cwd[CWD_SIZE];
	const char* path = getcwd(cwd, CWD_SIZE);
	if ( !path ) { path = "."; }

	if ( 1 < argc ) { path = argv[1]; }

	return ls(path);
}
