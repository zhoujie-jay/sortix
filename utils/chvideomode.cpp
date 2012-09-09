#include <sys/keycodes.h>
#include <sys/termmode.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <error.h>
#include <errno.h>
#include <termios.h>
#include <readparamstring.h>

bool SetCurrentMode(const char* mode)
{
	FILE* fp = fopen("/dev/video/mode", "w");
	if ( !fp ) { return false; }
	if ( fprintf(fp, "%s\n", mode) < 0 ) { fclose(fp); return false; }
	if ( fclose(fp) ) { return false; }
	return true;
}

char* GetCurrentMode()
{
	FILE* fp = fopen("/dev/video/mode", "r");
	if ( !fp ) { return NULL; }
	char* mode = NULL;
	size_t n = 0;
	getline(&mode, &n, fp);
	fclose(fp);
	return mode;
}

char** GetAvailableModes(size_t* nummodesptr)
{
	size_t modeslen = 0;
	char** modes = new char*[modeslen];
	if ( !modes ) { return NULL; }
	size_t nummodes = 0;
	FILE* fp = fopen("/dev/video/modes", "r");
	if ( !fp ) { delete[] modes; return NULL; }
	while ( true )
	{
		char* mode = NULL;
		size_t modesize = 0;
		ssize_t modelen = getline(&mode, &modesize, fp);
		if ( modelen <= 0 ) { break; }
		if ( mode[modelen-1] == '\n' ) { mode[modelen-1] = 0; }
		if ( nummodes == modeslen )
		{
			size_t newmodeslen = modeslen ? 2 * modeslen : 16UL;
			char** newmodes = new char*[newmodeslen];
			if ( !newmodes ) { free(mode); break; }
			memcpy(newmodes, modes, nummodes * sizeof(char*));
			delete[] modes; modes = newmodes;
			modeslen = newmodeslen;
		}
		modes[nummodes++] = mode;
	}
	fclose(fp);
	*nummodesptr = nummodes;
	return modes;
}

void DrawMenu(size_t selection, char** modes, size_t nummodes)
{
	printf("\e[m\e[H\e[2K"); // Move to (0,0), clear screen.
	printf("Please select one of these video modes or press ESC to abort.\n");
	size_t off = 1; // Above line
	struct winsize ws;
	if ( tcgetwinsize(1, &ws) != 0 )
		ws.ws_row = 25;
	size_t amount = ws.ws_row-off;
	size_t from = (selection / amount) * amount;
	size_t howmanyavailable = nummodes - from;
	size_t howmany = howmanyavailable < amount ? howmanyavailable : amount;
	size_t to = from + howmany - 1;
	for ( size_t i = from; i <= to; i++ )
	{
		size_t screenline = 1 + off + i - from;
		const char* color = i == selection ? "\e[31m" : "\e[m";
		printf("\e[%zuH%s\e[2K%zu\t%s", screenline, color, i, modes[i]);
	}
	printf("\e[m\e[J");
	fflush(stdout);
}

int GetKey(int fd)
{
	unsigned int oldtermmode;
	gettermmode(fd, &oldtermmode);
	// Read the keyboard input from the user.
	const unsigned termmode = TERMMODE_KBKEY
	                        | TERMMODE_UNICODE
	                        | TERMMODE_SIGNAL;
	if ( settermmode(fd, termmode) ) { error(1, errno, "settermmode"); }
	uint32_t codepoint;
	ssize_t numbytes;
	while ( 0 < (numbytes = read(0, &codepoint, sizeof(codepoint))) )
	{
		int kbkey = KBKEY_DECODE(codepoint);
		if ( !kbkey ) { continue; }
		return kbkey;
	}
	settermmode(fd, oldtermmode);
	return 0;
}

int GetKey(FILE* fp) { return GetKey(fileno(fp)); }

struct Filter
{
	bool includeall;
	bool includesupported;
	bool includeunsupported;
	bool includetext;
	bool includegraphics;
	size_t minbpp;
	size_t maxbpp;
	size_t minxres;
	size_t maxxres;
	size_t minyres;
	size_t maxyres;
	size_t minxchars;
	size_t maxxchars;
	size_t minychars;
	size_t maxychars;
};

size_t ParseSizeT(const char* str, size_t def = 0)
{
	if ( !str || !*str )
		return def;
	char* endptr;
	size_t ret = strtoul(str, &endptr, 10);
	if ( *endptr )
		return def;
	return ret;
}

bool ParseBool(const char* str, bool def = false)
{
	if ( !str || !*str )
		return def;
	bool isfalse = !strcmp(str, "0") || !strcmp(str, "false");
	return !isfalse;
}

bool PassesFilter(const char* modestr, Filter* filt)
{
	if ( filt->includeall ) { return true; }
	char* widthstr = NULL;
	char* heightstr = NULL;
	char* bppstr = NULL;
	char* unsupportedstr = NULL;
	char* textstr = NULL;
	if ( !ReadParamString(modestr,
	                      "width", &widthstr,
	                      "height", &heightstr,
	                      "bpp", &bppstr,
	                      "unsupported", &unsupportedstr,
	                      "text", &textstr,
	                      NULL) )
		error(1, errno, "Can't parse video mode: %s", modestr);
	size_t width = ParseSizeT(widthstr, 0); delete[] widthstr;
	size_t height = ParseSizeT(heightstr, 0); delete[] heightstr;
	size_t bpp = ParseSizeT(bppstr, 0); delete[] bppstr;
	bool unsupported = ParseBool(unsupportedstr); delete[] unsupportedstr;
	bool supported = !unsupported;
	bool text = ParseBool(textstr); delete[] textstr;
	bool graphics = !text;
	if ( unsupported && !filt->includeunsupported )
		return false;
	if ( supported && !filt->includesupported )
		return false;
	if ( text && !filt->includetext )
		return false;
	if ( graphics && !filt->includegraphics )
		return false;
	if ( graphics && (bpp < filt->minbpp || filt->maxbpp < bpp) )
		return false;
	if ( graphics && (width < filt->minxres || filt->maxxres < width) )
		return false;
	if ( graphics && (height < filt->minyres || filt->maxyres < height) )
		return false;
	// TODO: Support filtering text modes according to columns/rows.
	return true;
}

void FilterModes(char** modes, size_t* nummodesptr, Filter* filt)
{
	size_t innum = *nummodesptr;
	size_t outnum = 0;
	for ( size_t i = 0; i < innum; i++ )
	{
		if ( PassesFilter(modes[i], filt) )
			modes[outnum++] = modes[i];
		else
			delete[] modes[i];
	}
	*nummodesptr = outnum;
}

void Usage(FILE* fp, const char* argv0)
{
	fprintf(fp, "Usage: %s [OPTION ...] [PROGRAM-TO-RUN [ARG ...]]\n", argv0);
	fprintf(fp, "Changes the video mode and optionally runs a program\n");
	fprintf(fp, "\n");
	fprintf(fp, "Options supported by %s:\n", argv0);
	fprintf(fp, "  --help, --usage    Display this help and exit\n");
	fprintf(fp, "  --version          Output version information and exit\n");
	fprintf(fp, "\n");
	fprintf(fp, "Options for filtering modes:\n");
	fprintf(fp, "  --show-all BOOL\n");
	fprintf(fp, "  --show-supported BOOL, --show-unsupported BOOL\n");
	fprintf(fp, "  --show-text BOOL\n");
	fprintf(fp, "  --show-graphics BOOL\n");
	fprintf(fp, "  --bpp BPP, --min-bpp BPP, --max-bpp BPP\n");
	fprintf(fp, "  --width NUM, --min-width NUM, --max-width NUM\n");
	fprintf(fp, "  --height NUM, --min-height NUM, --max-height NUM\n");
	fprintf(fp, "\n");
}

void Help(FILE* fp, const char* argv0)
{
	Usage(fp, argv0);
}

void Version(FILE* fp, const char* argv0)
{
	Usage(fp, argv0);
}

bool SizeTParam(const char* name, const char* option,
                     const char* param, size_t* minvar, size_t* maxvar)
{
	char opt[64]; stpcpy(stpcpy(opt, "--"), name);
	char minopt[64]; stpcpy(stpcpy(minopt, "--min-"), name);
	char maxopt[64]; stpcpy(stpcpy(maxopt, "--max-"), name);
	if ( !strcmp(option, opt) )
		*minvar = *maxvar = ParseSizeT(param);
	else if ( !strcmp(option, minopt) )
		*minvar = ParseBool(param);
	else if ( !strcmp(option, maxopt) )
		*maxvar = ParseBool(param);
	else
		return false;
	return true;
}

bool BoolParam(const char* name, const char* option,
                     const char* param, bool* var)
{
	char opt[64]; stpcpy(stpcpy(opt, "--"), name);
	if ( !strcmp(option, opt) )
		*var = ParseBool(param);
	else
		return false;
	return true;
}

int main(int argc, char* argv[])
{
	const char* argv0 = argv[0];

	Filter filt;
	filt.includeall = false;
	filt.includesupported = true;
	filt.includeunsupported = false;
	filt.includetext = true;
	filt.includegraphics = true;
	// TODO: HACK: The kernel log printing requires either text mode or 32-bit
	// graphics. For now, just filter away anything but 32-bit graphics.
	filt.minbpp = 32;
	filt.maxbpp = 32;
	filt.minxres = 0;
	filt.maxxres = SIZE_MAX;
	filt.minyres = 0;
	filt.maxyres = SIZE_MAX;
	filt.minxchars = 0;
	filt.maxxchars = SIZE_MAX;
	filt.minychars = 0;
	filt.maxychars = SIZE_MAX;

	for ( int i = 1; i < argc; i++ )
	{
		const char* arg = argv[i];
		if ( arg[0] != '-' ) { break; }
		if ( !strcmp(arg, "--") ) { break; }
		if ( !strcmp(arg, "--help") ) { Help(stdout, argv0); exit(0); }
		if ( !strcmp(arg, "--usage") ) { Usage(stdout, argv0); exit(0); }
		if ( !strcmp(arg, "--version") ) { Version(stdout, argv0); exit(0); }
		if ( i == argc-1 )
		{
			fprintf(stderr, "%s: missing parameter to %s\n", argv0, arg);
			Usage(stderr, argv0);
			exit(1);
		}
		const char* p = argv[++i]; argv[i] = NULL;
		bool handled =
			BoolParam("show-all", arg, p, &filt.includeall) ||
			BoolParam("show-supported", arg, p, &filt.includesupported) ||
			BoolParam("show-unsupported", arg, p, &filt.includeunsupported) ||
			BoolParam("show-text", arg, p, &filt.includetext) ||
			BoolParam("show-graphics", arg, p, &filt.includegraphics) ||
			SizeTParam("bpp", arg, p, &filt.minbpp, &filt.maxbpp) ||
			SizeTParam("width", arg, p, &filt.minxres, &filt.maxxres) ||
			SizeTParam("height", arg, p, &filt.minyres, &filt.maxyres) ||
			false;
		if ( !handled )
		{
			fprintf(stderr, "%s: no such option: %s\n", argv0, arg);
			Usage(stderr, argv0);
			exit(1);
		}
	}

	char* prevmode = GetCurrentMode();
	if ( !prevmode ) { perror("Unable to detect current mode"); exit(1); }
#if 1
	size_t nummodes = 0;
	char** modes = GetAvailableModes(&nummodes);
	if ( !modes ) { perror("Unable to detect available video modes"); exit(1); }

	if ( !nummodes )
	{
		fprintf(stderr, "No video modes are currently available.\n");
		fprintf(stderr, "Try make sure a driver exists and is activated.\n");
		exit(1);
	}

	FilterModes(modes, &nummodes, &filt);
	if ( !nummodes )
	{
		fprintf(stderr, "\
No video mode remains after filtering away unwanted modes.\n");
		fprintf(stderr, "\
Try make sure the desired driver is loaded and is configured correctly.\n");
		exit(1);
	}

	size_t selection = 0;
	bool decided = false;
	while ( !decided )
	{
		DrawMenu(selection, modes, nummodes);
		switch ( GetKey(stdin) )
		{
		case KBKEY_ESC: printf("\n"); exit(10); break;
		case KBKEY_UP: selection = (nummodes+selection-1) % nummodes; break;
		case KBKEY_DOWN: selection = (selection+1) % nummodes; break;
		case KBKEY_ENTER: decided = true; break;
		}
	}

	const char* mode = modes[selection];
	if ( !SetCurrentMode(mode) )
	{
		error(1, errno, "Unable to set video mode: %s", mode);
	}
#endif
	if ( 1 < argc )
	{
		pid_t childpid = fork();
		if ( childpid < 0 ) { perror("fork"); exit(1); }
		if ( childpid )
		{
			int status;
			waitpid(childpid, &status, 0);
			if ( !SetCurrentMode(prevmode) )
			{
				error(1, errno, "Unable to restore video mode: %s", prevmode);
			}
			exit(WEXITSTATUS(status));
		}
		execvp(argv[1], argv + 1);
		perror(argv[1]);
	}

	return 0;
}
