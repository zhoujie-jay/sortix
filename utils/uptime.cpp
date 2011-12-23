#include <stdio.h>
#include <unistd.h>

size_t Seconds(uintmax_t usecs)
{
	return (usecs / (1000ULL * 1000ULL)) % (60ULL);
}

size_t Minutes(uintmax_t usecs)
{
	return (usecs / (1000ULL * 1000ULL * 60ULL)) % (60ULL);
}

size_t Hours(uintmax_t usecs)
{
	return (usecs / (1000ULL * 1000ULL * 60ULL * 60ULL)) % (24ULL);
}

size_t Days(uintmax_t usecs)
{
	return usecs / (1000ULL * 1000ULL * 60ULL * 60ULL * 24ULL);
}

void PrintElement(size_t num, const char* single, const char* multiple)
{
	static const char* prefix = "";
	if ( !num ) { return; }
	const char* str = (num>1) ? multiple : single;
	printf("%s%zu %s", prefix, num, str);
	prefix = ", ";
}

int main(int argc, char* argv[])
{
	uintmax_t usecssinceboot;
	if ( uptime(&usecssinceboot) ) { perror("uptime"); return 1; }

	PrintElement(Days(usecssinceboot), "day", "days");
	PrintElement(Hours(usecssinceboot), "hour", "hours");
	PrintElement(Minutes(usecssinceboot), "min", "mins");
	PrintElement(Seconds(usecssinceboot), "sec", "secs");
	printf("\n");

	return 0;
}
