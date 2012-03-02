#include <stdio.h>
#include <errno.h>
#include <error.h>
#include <unistd.h>

int main(int argc, char* argv[])
{
	printf("Please enter the name of one of the following programs:\n");

	const char* programname = "ls";
	const char* newargv[] = { programname, "/bin", NULL };
	execv(programname, (char* const*) newargv);
	error(1, errno, "%s", programname);

	return 1;
}
