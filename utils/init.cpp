#include <stdio.h>
#include <libmaxsi/process.h>

int main(int argc, char* argv[])
{
	// Reset the terminal's color and the rest of it.
	printf("\e[m\e[J");

	const char* programname = "sh";
	const char* newargv[] = { programname };

	Maxsi::Process::Execute(programname, 1, newargv);

	return 1;
}
