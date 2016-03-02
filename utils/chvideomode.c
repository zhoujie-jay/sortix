/*
 * Copyright (c) 2012, 2013, 2014 Jonas 'Sortie' Termansen.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * chvideomode.c
 * Menu for changing the screen resolution.
 */

#include <sys/display.h>
#include <sys/keycodes.h>
#include <sys/termmode.h>
#include <sys/wait.h>

#include <errno.h>
#include <error.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

bool SetCurrentMode(struct dispmsg_crtc_mode mode)
{
	struct dispmsg_set_crtc_mode msg;
	msg.msgid = DISPMSG_SET_CRTC_MODE;
	msg.device = 0;
	msg.connector = 0;
	msg.mode = mode;
	return dispmsg_issue(&msg, sizeof(msg)) == 0;
}

struct dispmsg_crtc_mode* GetAvailableModes(size_t* num_modes_ptr)
{
	struct dispmsg_get_crtc_modes msg;
	msg.msgid = DISPMSG_GET_CRTC_MODES;
	msg.device = 0;
	msg.connector = 0;
	size_t guess = 1;
	while ( true )
	{
		struct dispmsg_crtc_mode* ret = (struct dispmsg_crtc_mode*)
			malloc(sizeof(struct dispmsg_crtc_mode) * guess);
		if ( !ret )
			return NULL;
		msg.modes_length = guess;
		msg.modes = ret;
		if ( dispmsg_issue(&msg, sizeof(msg)) == 0 )
		{
			*num_modes_ptr = guess;
			return ret;
		}
		free(ret);
		if ( errno == ERANGE && guess < msg.modes_length )
		{
			guess = msg.modes_length;
			continue;
		}
		return NULL;
	}
}

struct filter
{
	bool include_all;
	bool include_supported;
	bool include_unsupported;
	bool include_text;
	bool include_graphics;
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

bool mode_passes_filter(struct dispmsg_crtc_mode mode, struct filter* filter)
{
	if ( filter->include_all )
		return true;
	size_t width = mode.view_xres;
	size_t height = mode.view_yres;
	size_t bpp = mode.fb_format;
	bool supported = (mode.control & DISPMSG_CONTROL_VALID) ||
	                 (mode.control & DISPMSG_CONTROL_OTHER_RESOLUTIONS);
	bool unsupported = !supported;
	bool text = mode.control & DISPMSG_CONTROL_VGA;
	bool graphics = !text;
	if ( mode.control & DISPMSG_CONTROL_OTHER_RESOLUTIONS )
		return true;
	if ( unsupported && !filter->include_unsupported )
		return false;
	if ( supported && !filter->include_supported )
		return false;
	if ( text && !filter->include_text )
		return false;
	if ( graphics && !filter->include_graphics )
		return false;
	if ( graphics && (bpp < filter->minbpp || filter->maxbpp < bpp) )
		return false;
	if ( graphics && (width < filter->minxres || filter->maxxres < width) )
		return false;
	if ( graphics && (height < filter->minyres || filter->maxyres < height) )
		return false;
	// TODO: Support filtering text modes according to columns/rows.
	return true;
}

void filter_modes(struct dispmsg_crtc_mode* modes, size_t* num_modes_ptr, struct filter* filter)
{
	size_t in_num = *num_modes_ptr;
	size_t out_num = 0;
	for ( size_t i = 0; i < in_num; i++ )
	{
		if ( mode_passes_filter(modes[i], filter) )
			modes[out_num++] = modes[i];
	}
	*num_modes_ptr = out_num;
}

size_t parse_size_t(const char* str, size_t def)
{
	if ( !str || !*str )
		return def;
	char* endptr;
	size_t ret = (size_t) strtoumax(str, &endptr, 10);
	if ( *endptr )
		return def;
	return ret;
}

bool parse_bool(const char* str, bool def)
{
	if ( !str || !*str )
		return def;
	bool isfalse = !strcmp(str, "0") || !strcmp(str, "false");
	return !isfalse;
}

static void compact_arguments(int* argc, char*** argv)
{
	for ( int i = 0; i < *argc; i++ )
	{
		while ( i < *argc && !(*argv)[i] )
		{
			for ( int n = i; n < *argc; n++ )
				(*argv)[n] = (*argv)[n+1];
			(*argc)--;
		}
	}
}

static void help(FILE* fp, const char* argv0)
{
	fprintf(fp, "Usage: %s [OPTION ...] [-- PROGRAM-TO-RUN [ARG ...]]\n", argv0);
	fprintf(fp, "Changes the video mode and optionally runs a program\n");
	fprintf(fp, "\n");
	fprintf(fp, "Options supported by %s:\n", argv0);
	fprintf(fp, "  --help             Display this help and exit\n");
	fprintf(fp, "  --version          Output version information and exit\n");
	fprintf(fp, "\n");
	fprintf(fp, "Options for filtering modes:\n");
	fprintf(fp, "  --show-all=BOOL\n");
	fprintf(fp, "  --show-supported=BOOL, --show-unsupported=BOOL\n");
	fprintf(fp, "  --show-text=BOOL\n");
	fprintf(fp, "  --show-graphics=BOOL\n");
	fprintf(fp, "  --bpp BPP, --min-bpp=BPP, --max-bpp=BPP\n");
	fprintf(fp, "  --width=NUM, --min-width=NUM, --max-width=NUM\n");
	fprintf(fp, "  --height=NUM, --min-heigh= NUM, --max-height=NUM\n");
}

static void version(FILE* fp, const char* argv0)
{
	fprintf(fp, "%s (Sortix) %s\n", argv0, VERSIONSTR);
}

bool string_parameter(const char* option,
                      const char* arg,
                      int argc,
                      char** argv,
                      int* ip,
                      const char* argv0,
                      const char** result)
{
	size_t option_len = strlen(option);
	if ( strncmp(option, arg, option_len) != 0 )
		return false;
	if ( arg[option_len] == '=' )
		return *result = arg + option_len + 1, true;
	if ( arg[option_len] != '\0' )
		return false;
	if ( *ip + 1 == argc )
	{
		fprintf(stderr, "%s: expected operand after `%s'\n", argv0, option);
		exit(1);
	}
	*result = argv[++*ip];
	argv[*ip] = NULL;
	return true;
}

bool minmax_parameter(const char* option,
                      const char* min_option,
                      const char* max_option,
                      const char* arg,
                      int argc,
                      char** argv,
                      int* ip,
                      const char* argv0,
                      size_t* min_result,
                      size_t* max_result)
{
	const char* parameter;
	if ( string_parameter(option, arg, argc, argv, ip, argv0, &parameter) )
		return *min_result = *max_result = parse_size_t(parameter, 0), true;
	if ( string_parameter(min_option, arg, argc, argv, ip, argv0, &parameter) )
		return *min_result = parse_size_t(parameter, 0), true;
	if ( string_parameter(max_option, arg, argc, argv, ip, argv0, &parameter) )
		return *max_result = parse_size_t(parameter, 0), true;
	return false;
}

#define MINMAX_PARAMETER(option, min_result, max_result) \
        minmax_parameter("--" option, "--min-" option, "--max" option, arg, \
                         argc, argv, &i, argv0, min_result, max_result)

bool bool_parameter(const char* option,
                    const char* arg,
                    int argc,
                    char** argv,
                    int* ip,
                    const char* argv0,
                    bool* result)
{
	const char* parameter;
	if ( string_parameter(option, arg, argc, argv, ip, argv0, &parameter) )
		return *result = parse_bool(parameter, false), true;
	return false;
}

#define BOOL_PARAMETER(option, result) \
        bool_parameter("--" option, arg, argc, argv, &i, argv0, result)

int main(int argc, char* argv[])
{
	struct filter filter;

	filter.include_all = false;
	filter.include_supported = true;
	filter.include_unsupported = false;
	filter.include_text = true;
	filter.include_graphics = true;
	// TODO: HACK: The kernel log printing requires either text mode or 32-bit
	// graphics. For now, just filter away anything but 32-bit graphics.
	filter.minbpp = 32;
	filter.maxbpp = 32;
	filter.minxres = 0;
	filter.maxxres = SIZE_MAX;
	filter.minyres = 0;
	filter.maxyres = SIZE_MAX;
	filter.minxchars = 0;
	filter.maxxchars = SIZE_MAX;
	filter.minychars = 0;
	filter.maxychars = SIZE_MAX;

	const char* argv0 = argv[0];
	for ( int i = 1; i < argc; i++ )
	{
		const char* arg = argv[i];
		if ( arg[0] != '-' || !arg[1] )
			break; // Intentionally not continue.
		argv[i] = NULL;
		if ( !strcmp(arg, "--") )
			break;
		if ( arg[1] != '-' )
		{
			char c;
			while ( (c = *++arg) ) switch ( c )
			{
			default:
				fprintf(stderr, "%s: unknown option -- '%c'\n", argv0, c);
				help(stderr, argv0);
				exit(1);
			}
		}
		else if ( !strcmp(arg, "--help") )
			help(stdout, argv0), exit(0);
		else if ( !strcmp(arg, "--version") )
			version(stdout, argv0), exit(0);
		else if ( BOOL_PARAMETER("show-all", &filter.include_all) ) { }
		else if ( BOOL_PARAMETER("show-supported", &filter.include_supported) ) { }
		else if ( BOOL_PARAMETER("show-unsupported", &filter.include_unsupported) ) { }
		else if ( BOOL_PARAMETER("show-text", &filter.include_text) ) { }
		else if ( BOOL_PARAMETER("show-graphics", &filter.include_graphics) ) { }
		else if ( MINMAX_PARAMETER("bpp", &filter.minbpp, &filter.maxbpp) ) { }
		else if ( MINMAX_PARAMETER("width", &filter.minxres, &filter.maxxres) ) { }
		else if ( MINMAX_PARAMETER("height", &filter.minyres, &filter.maxyres) ) { }
		else
		{
			fprintf(stderr, "%s: unknown option: %s\n", argv0, arg);
			help(stderr, argv0);
			exit(1);
		}
	}

	compact_arguments(&argc, &argv);

	size_t num_modes = 0;
	struct dispmsg_crtc_mode* modes = GetAvailableModes(&num_modes);
	if ( !modes )
		error(1, errno, "Unable to detect available video modes");

	if ( !num_modes )
	{
		fprintf(stderr, "No video modes are currently available.\n");
		fprintf(stderr, "Try make sure a device driver exists and is activated.\n");
		exit(11);
	}

	filter_modes(modes, &num_modes, &filter);
	if ( !num_modes )
	{
		fprintf(stderr, "No video mode remains after filtering away unwanted modes.\n");
		fprintf(stderr, "Try make sure the desired device driver is loaded and is configured correctly.\n");
		exit(12);
	}

	int num_modes_display_length = 1;
	for ( size_t i = num_modes; 10 <= i; i /= 10 )
		num_modes_display_length++;

	int mode_set_error = 0;
	size_t selection;
	bool decided;
	bool first_render;
	struct wincurpos render_at;
retry_pick_mode:
	selection = 0;
	decided = false;
	first_render = true;
	memset(&render_at, 0, sizeof(render_at));
	while ( !decided )
	{
		fflush(stdout);

		struct winsize ws;
		if ( tcgetwinsize(1, &ws) != 0 )
		{
			ws.ws_col = 80;
			ws.ws_row = 25;
		}

		struct wincurpos wcp;
		if ( tcgetwincurpos(1, &wcp) != 0 )
		{
			wcp.wcp_col = 1;
			wcp.wcp_row = 1;
		}

		size_t off = 1; // The "Please select ..." line at the top.
		if ( mode_set_error )
			off++;

		size_t entries_per_page = ws.ws_row - off;
		size_t page = selection / entries_per_page;
		size_t from = page * entries_per_page;
		size_t how_many_available = num_modes - from;
		size_t how_many = entries_per_page;
		if ( how_many_available < how_many )
			how_many = how_many_available;
		size_t lines_on_screen = off + how_many;

		if ( first_render )
		{
			while ( wcp.wcp_row && ws.ws_row - (wcp.wcp_row + 1) < lines_on_screen )
			{
				printf("\e[S");
				printf("\e[%juH", 1 + (uintmax_t) wcp.wcp_row);
				wcp.wcp_row--;
				wcp.wcp_col = 1;
			}
			render_at = wcp;
			first_render = false;
		}

		printf("\e[m");
		printf("\e[%juH", 1 + (uintmax_t) render_at.wcp_row);
		printf("\e[2K");

		if ( mode_set_error )
			printf("Error: Could not set desired mode: %s\n", strerror(mode_set_error));
		printf("Please select one of these video modes or press ESC to abort.\n");

		for ( size_t i = 0; i < how_many; i++ )
		{
			size_t index = from + i;
			size_t screenline = off + index - from;
			const char* color = index == selection ? "\e[31m" : "\e[m";
			printf("\e[%zuH", 1 + render_at.wcp_row + screenline);
			printf("%s", color);
			printf("\e[2K");
			printf(" [%-*zu] ", num_modes_display_length, index);
			if ( modes[i].control & DISPMSG_CONTROL_VALID )
				printf("%u x %u x %u",
					modes[i].fb_format,
					modes[i].view_xres,
					modes[i].view_yres);
			else if ( modes[i].control & DISPMSG_CONTROL_OTHER_RESOLUTIONS )
				printf("(enter a custom resolution)");
			else
				printf("(unknown video device feature)");
			printf("\e[m");
		}

		printf("\e[J");
		fflush(stdout);

		unsigned int oldtermmode;
		if ( gettermmode(0, &oldtermmode) < 0 )
			error(1, errno, "gettermmode");

		if ( settermmode(0, TERMMODE_KBKEY | TERMMODE_UNICODE | TERMMODE_SIGNAL) < 0 )
			error(1, errno, "settermmode");

		bool redraw = false;
		while ( !redraw && !decided )
		{
			uint32_t codepoint;
			ssize_t numbytes = read(0, &codepoint, sizeof(codepoint));
			if ( numbytes < 0 )
				error(1, errno, "read");

			int kbkey = KBKEY_DECODE(codepoint);
			if ( kbkey )
			{
				switch ( kbkey )
				{
				case KBKEY_ESC:
					if ( settermmode(0, oldtermmode) < 0 )
						error(1, errno, "settermmode");
					printf("\n");
					exit(10);
					break;
				case KBKEY_UP:
					if ( selection )
						selection--;
					else
						selection = num_modes -1;
					redraw = true;
					break;
				case KBKEY_DOWN:
					if ( selection + 1 == num_modes )
						selection = 0;
					else
						selection++;
					redraw = true;
					break;
				case KBKEY_ENTER:
					if ( settermmode(0, oldtermmode) < 0 )
						error(1, errno, "settermmode");
					fgetc(stdin);
					printf("\n");
					decided = true;
					break;
				}
			}
			else
			{
				if ( L'0' <= codepoint && codepoint <= '9' )
				{
					uint32_t requested = codepoint - '0';
					if ( requested < num_modes )
					{
						selection = requested;
						redraw = true;
					}
				}
			}
		}

		if ( settermmode(0, oldtermmode) < 0 )
			error(1, errno, "settermmode");
	}

	struct dispmsg_crtc_mode mode = modes[selection];
	if ( mode.control & DISPMSG_CONTROL_OTHER_RESOLUTIONS )
	{
		uintmax_t req_bpp;
		uintmax_t req_width;
		uintmax_t req_height;
		while ( true )
		{
			printf("Enter video mode [BPP x WIDTH x HEIGHT]: ");
			fflush(stdout);
			if ( scanf("%ju x %ju x %ju", &req_bpp, &req_width, &req_height) != 3 )
			{
				fgetc(stdin);
				fflush(stdin);
				continue;
			}
			fgetc(stdin);
			break;
		}
		mode.fb_format = req_bpp;
		mode.view_xres = req_width;
		mode.view_yres = req_height;
		mode.control &= ~DISPMSG_CONTROL_OTHER_RESOLUTIONS;
		mode.control |= DISPMSG_CONTROL_VALID;
	}

	if ( !SetCurrentMode(mode) )
	{
		error(0, mode_set_error = errno, "Unable to set video mode %ju x %ju x %ju",
			(uintmax_t) mode.fb_format,
			(uintmax_t) mode.view_xres,
			(uintmax_t) mode.view_yres);
		goto retry_pick_mode;
	}

	if ( 1 < argc )
	{
		execvp(argv[1], argv + 1);
		error(127, errno, "`%s'", argv[1]);
	}

	return 0;
}
