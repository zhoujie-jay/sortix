#include <sys/keycodes.h>
#include <sys/termmode.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <error.h>

int docat(const char* inputname, int fd)
{
	do
	{
		const size_t BUFFER_SIZE = 255;
		char buffer[BUFFER_SIZE+1];
		ssize_t bytesread = read(fd, buffer, BUFFER_SIZE);
		if ( bytesread == 0 ) { break; }
		if ( bytesread < 0 )
		{
			error(0, errno, "read: %s", inputname);
			return 1;
		}
		if ( writeall(1, buffer, bytesread) )
		{
			error(0, errno, "write: %s", inputname);
			return 1;
		}
	} while ( true );
	return 0;
}

int cat(int argc, char* argv[])
{
	int result = 0;

	for ( int i = 1; i < argc; i++ )
	{
		int fd = open(argv[i], O_RDONLY);
		if ( fd < 0 )
		{
			error(0, errno, "%s", argv[i]);
			result = 1;
			continue;
		}
		
		result |= docat(argv[i], fd);
		close(fd);
	}

	return result;
}

int main(int argc, char* argv[])
{
	if ( 1 < argc )
	{
		return cat(argc, argv);
	}

	if ( !isatty(0) ) { return docat("<stdin>", 0); }

	bool lastwasesc = false;

	// TODO: This is just compatibility with how cat worked in early versions of
	// Sortix. Ideally, this should be removed and just cat the raw stdin.
	// Read the keyboard input from the user.
	unsigned termmode = TERMMODE_KBKEY | TERMMODE_UNICODE | TERMMODE_SIGNAL;
	if ( settermmode(0, termmode) ) { error(1, errno, "settermmode"); }
	while ( true )
	{
		uint32_t codepoint;
		ssize_t numbytes = read(0, &codepoint, sizeof(codepoint));
		if ( !numbytes ) { break; }
		if ( numbytes < 0 ) { break; }
		int kbkey = KBKEY_DECODE(codepoint);
		if ( kbkey < 0 ) { continue; }
		if ( kbkey == KBKEY_UP ) { printf("\e[A"); fflush(stdout); continue; }
		if ( kbkey == KBKEY_DOWN ) { printf("\e[B"); fflush(stdout); continue; }
		if ( kbkey == KBKEY_RIGHT ) { printf("\e[C"); fflush(stdout); continue; }
		if ( kbkey == KBKEY_LEFT ) { printf("\e[D"); fflush(stdout); continue; }
		if ( kbkey == KBKEY_ESC ) { printf("\e["); fflush(stdout); lastwasesc = true; continue; }
		if ( kbkey ) { continue; }
		if ( lastwasesc && codepoint == '[' ) { continue; }

		if ( codepoint >= 0x80 ) { continue; }

		char msg[2]; msg[0] = codepoint; msg[1] = '\0';
		printf(msg);
		fflush(stdout);
		lastwasesc = false;
	}

	return 0;
}

