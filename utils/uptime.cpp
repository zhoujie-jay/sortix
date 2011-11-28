#include <stdio.h>
#include <unistd.h>

size_t Seconds(size_t ms)
{
	return (ms / 1000UL) % (60UL);
}

size_t Minutes(size_t ms)
{
	return (ms / (1000UL * 60UL)) % (60UL);
}

size_t Hours(size_t ms)
{
	return (ms / (1000UL * 60UL * 60UL)) % (24UL);
}

size_t Days(size_t ms)
{
	return (ms / (1000UL * 60UL * 60UL * 24UL)) % (24UL);
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
	uintmax_t mssinceboot;
	if ( uptime(&mssinceboot) ) { perror("uptime"); return 1; }

	PrintElement(Days(mssinceboot), "day", "days");
	PrintElement(Hours(mssinceboot), "hour", "hours");
	PrintElement(Minutes(mssinceboot), "min", "mins");
	PrintElement(Seconds(mssinceboot), "sec", "secs");
	printf("\n");

	return 0;
}
