#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <error.h>
#include <dirent.h>

int ls(const char* path)
{
	DIR* dir = opendir(path);
	if ( !dir ) { error(2, errno, "%s", path); return 2; }

	// TODO: Hack until mountpoints work correctly.
	if ( strcmp(path, "/") == 0 ) { printf("bin\ndev\n"); }

	struct dirent* entry;
	while ( (entry = readdir(dir)) )
	{
		printf("%s\n", entry->d_name);
	}

	if ( derror(dir) )
	{
		error(2, errno, "readdir: %s", path);
	}

	closedir(dir);

	return 0;
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
