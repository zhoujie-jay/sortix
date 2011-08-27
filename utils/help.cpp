#include <stdio.h>
#include <libmaxsi/process.h>

int main(int argc, char* argv[])
{
	// Reset the terminal's color and the rest of it.
	printf("Please enter the name of one of the following programs:\n");

	const char* programname = "ls";
	const char* newargv[] = { programname };

	Maxsi::Process::Execute(programname, 1, newargv);

	return 1;
}
