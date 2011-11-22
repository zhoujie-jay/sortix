#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

bool writeall(int fd, const void* buffer, size_t len)
{
	const char* buf = (const char*) buffer;
	while ( len )
	{
		ssize_t byteswritten = write(fd, buf, len);
		if ( byteswritten < 0 ) { return false; }
		buf += byteswritten;
		len -= byteswritten;
	}

	return true;
}

int main(int argc, char* argv[])
{
	if ( argc != 3 ) { printf("usage: %s <from> <to>\n", argv[0]); return 0; }

	int fromfd = open(argv[1], O_RDONLY);
	if ( fromfd < 0 ) { printf("%s: %s: %s\n", argv[0], argv[1], strerror(errno)); return 1; }

	int tofd = open(argv[2], O_WRONLY | O_TRUNC | O_CREAT, 0777);
	if ( tofd < 0 ) { printf("%s: %s: %s\n", argv[0], argv[2], strerror(errno)); return 1; }

	while ( true )
	{
		const size_t BUFFER_SIZE = 4096;
		char buffer[BUFFER_SIZE];
		ssize_t bytesread = read(fromfd, buffer, BUFFER_SIZE);
		if ( bytesread < 0 ) { printf("%s: %s: %s\n", argv[0], argv[1], strerror(errno)); return 1; }
		if ( bytesread == 0 ) { return 0; }
		if ( !writeall(tofd, buffer, bytesread) ) { printf("%s: %s: %s\n", argv[0], argv[2], strerror(errno)); return 1; }
	}
}
