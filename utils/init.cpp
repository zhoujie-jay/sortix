#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <libmaxsi/platform.h>
#include <libmaxsi/process.h>
#include <libmaxsi/thread.h>

using namespace Maxsi;

int parent(pid_t childid)
{
	int status;
	waitpid(childid, &status, 0);
	return status;
}

int child()
{
	const char* programname = "sh";
	const char* newargv[] = { programname };

	Process::Execute(programname, 1, newargv);

	error(0, errno, "%s", programname);

	return 2;
}

int main(int argc, char* argv[])
{
	if ( open("/dev/tty", O_RDONLY) != 0 ) { return 2; }
	if ( open("/dev/tty", O_WRONLY | O_APPEND) != 1 ) { return 2; }
	if ( open("/dev/tty", O_WRONLY | O_APPEND) != 2 ) { return 2; }

	// Reset the terminal's color and the rest of it.
	printf("\r\e[m\e[J");
	fflush(stdout);

	pid_t childpid = Process::Fork();	
	if ( childpid < 0 ) { perror("fork"); return 2; }

	return ( childpid == 0 ) ? child() : parent(childpid);
}
