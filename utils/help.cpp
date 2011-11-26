#include <stdio.h>
#include <errno.h>
#include <error.h>
#include <libmaxsi/platform.h>
#include <libmaxsi/process.h>

int main(int argc, char* argv[])
{
	// Reset the terminal's color and the rest of it.
	printf("Please enter the name of one of the following programs:\n");

	const char* programname = "ls";
	const char* newargv[] = { programname, "/bin" };

	Maxsi::Process::Execute(programname, 2, newargv);

	error(1, errno, "%s", programname);

	return 1;
}
