/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012, 2013.

    This program is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
    more details.

    You should have received a copy of the GNU General Public License along with
    this program. If not, see <http://www.gnu.org/licenses/>.

    chvideomode.cpp
    Menu for changing the screen resolution.

*******************************************************************************/

#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS

#include <sys/display.h>
#include <sys/keycodes.h>
#include <sys/termmode.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <error.h>
#include <errno.h>
#include <termios.h>

// TODO: Provide this using asprintf or somewhere sane in user-space.
char* Combine(size_t NumParameters, ...)
{
	va_list param_pt;

	va_start(param_pt, NumParameters);

	// First calculate the string length.
	size_t ResultLength = 0;
	const char* TMP = 0;

	for ( size_t I = 0; I < NumParameters; I++ )
	{
		TMP = va_arg(param_pt, const char*);

		if ( TMP != NULL ) { ResultLength += strlen(TMP); }
	}

	// Allocate a string with the desired length.
	char* Result = new char[ResultLength+1];
	if ( !Result ) { return NULL; }

	Result[0] = 0;

	va_end(param_pt);
	va_start(param_pt, NumParameters);

	size_t ResultOffset = 0;

	for ( size_t I = 0; I < NumParameters; I++ )
	{
		TMP = va_arg(param_pt, const char*);

		if ( TMP )
		{
			size_t TMPLength = strlen(TMP);
			strcpy(Result + ResultOffset, TMP);
			ResultOffset += TMPLength;
		}
	}

	return Result;
}

char* ActualGetDriverName(uint64_t driver_index)
{
	struct dispmsg_get_driver_name msg;
	msg.msgid = DISPMSG_GET_DRIVER_NAME;
	msg.device = 0;
	msg.driver_index = driver_index;
	size_t guess = 32;
	while ( true )
	{
		char* ret = new char[guess];
		if ( !ret )
			return NULL;
		msg.name.byte_size = guess;
		msg.name.str = ret;
		if ( dispmsg_issue(&msg, sizeof(msg)) == 0 )
			return ret;
		delete[] ret;
		if ( errno == ERANGE && guess < msg.name.byte_size )
		{
			guess = msg.name.byte_size;
			continue;
		}
		return NULL;
	}
}

static char* StringOfCrtcMode(struct dispmsg_crtc_mode mode)
{
	char bppstr[sizeof(mode.fb_format) * 3];
	char xresstr[sizeof(mode.view_xres) * 3];
	char yresstr[sizeof(mode.view_yres) * 3];
	char magicstr[sizeof(mode.magic) * 3];
	snprintf(bppstr, sizeof(bppstr), "%ju", (uintmax_t) mode.fb_format);
	snprintf(xresstr, sizeof(xresstr), "%ju", (uintmax_t) mode.view_xres);
	snprintf(yresstr, sizeof(yresstr), "%ju", (uintmax_t) mode.view_yres);
	snprintf(magicstr, sizeof(magicstr), "%ju", (uintmax_t) mode.magic);
	char* drivername = ActualGetDriverName(mode.driver_index);
	if ( !drivername )
		return NULL;
	char* ret = Combine(10,
	                    "driver=", drivername, ","
	                    "bpp=", bppstr, ","
	                    "width=", xresstr, ","
	                    "height=", yresstr, ","
	                    "modeid=", magicstr);
	delete[] drivername;
	return ret;
}

bool SetCurrentMode(struct dispmsg_crtc_mode mode)
{
	struct dispmsg_set_crtc_mode msg;
	msg.msgid = DISPMSG_SET_CRTC_MODE;
	msg.device = 0;
	msg.connector = 0;
	msg.mode = mode;
	return dispmsg_issue(&msg, sizeof(msg)) == 0;
}

struct dispmsg_crtc_mode GetCurrentMode()
{
	struct dispmsg_set_crtc_mode msg;
	msg.msgid = DISPMSG_GET_CRTC_MODE;
	msg.device = 0;
	msg.connector = 0;
	dispmsg_issue(&msg, sizeof(msg));
	return msg.mode;
}

struct dispmsg_crtc_mode* GetAvailableModes(size_t* nummodesptr)
{
	struct dispmsg_get_crtc_modes msg;
	msg.msgid = DISPMSG_GET_CRTC_MODES;
	msg.device = 0;
	msg.connector = 0;
	size_t guess = 0;
	while ( true )
	{
		struct dispmsg_crtc_mode* ret = new struct dispmsg_crtc_mode[guess];
		if ( !ret )
			return NULL;
		msg.modes_length = guess;
		msg.modes = ret;
		if ( dispmsg_issue(&msg, sizeof(msg)) == 0 )
		{
			*nummodesptr = guess;
			return ret;
		}
		delete[] ret;
		if ( errno == ERANGE && guess < msg.modes_length )
		{
			guess = msg.modes_length;
			continue;
		}
		return NULL;
	}
}

void DrawMenu(size_t selection, struct dispmsg_crtc_mode* modes, size_t nummodes)
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
		char* desc = StringOfCrtcMode(modes[i]);
		const char* print_desc = desc ? desc : "<unknown - error>";
		printf("\e[%zuH%s\e[2K%zu\t%s", screenline, color, i, print_desc);
		delete[] desc;
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

bool PassesFilter(struct dispmsg_crtc_mode mode, Filter* filt)
{
	if ( filt->includeall ) { return true; }
	size_t width = mode.view_xres;
	size_t height = mode.view_yres;
	size_t bpp = mode.fb_format;
	bool unsupported = (mode.control & 1) == 0;
	bool supported = !unsupported;
	bool text = false; // TODO: How does <sys/display.h> tell this?
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

void FilterModes(struct dispmsg_crtc_mode* modes, size_t* nummodesptr, Filter* filt)
{
	size_t innum = *nummodesptr;
	size_t outnum = 0;
	for ( size_t i = 0; i < innum; i++ )
	{
		if ( PassesFilter(modes[i], filt) )
			modes[outnum++] = modes[i];
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

	struct dispmsg_crtc_mode prevmode = GetCurrentMode();
#if 1
	size_t nummodes = 0;
	struct dispmsg_crtc_mode* modes = GetAvailableModes(&nummodes);
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

	struct dispmsg_crtc_mode mode = modes[selection];
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
		error(127, errno, "`%s'", argv[1]);
	}

	return 0;
}
