#include <stdio.h>
#include <string.h>

int main(int argc, char* argv[])
{
	int startfrom = 1;
	bool trailingnewline = true;
	if ( 1 < argc && strcmp(argv[1], "-n") == 0 )
	{
		trailingnewline = false;
		startfrom = 2;
	}

	const char* prefix = "";
	for ( int i = startfrom; i < argc; i++ )
	{
		printf("%s%s", prefix, argv[i]);
		prefix = " ";
	}

	if ( trailingnewline ) { printf("\n"); }

	return 0;
}
