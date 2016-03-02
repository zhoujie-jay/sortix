/*
 * Copyright (c) 2014, 2015 Jonas 'Sortie' Termansen.
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
 * pager.c
 * Displays files one page at a time.
 */

#include <sys/keycodes.h>
#include <sys/termmode.h>

#include <assert.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <ioleast.h>
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <wchar.h>

#define CONTROL_SEQUENCE_MAX 128

enum control_state
{
	CONTROL_STATE_NONE = 0,
	CONTROL_STATE_CSI,
	CONTROL_STATE_COMMAND,
};

struct line
{
	char* content;
	size_t content_used;
	size_t content_length;
};

struct pager
{
	int tty_fd;
	unsigned int tty_fd_old_termmode;
	int out_fd;
	bool out_fd_is_tty;
	struct winsize out_fd_winsize;
	mbstate_t in_ps;
	mbstate_t out_ps;
	const char* input_prompt_name;
	size_t possible_lines;
	size_t allowed_lines;
	bool quiting;
	bool flag_raw_control_chars;
	bool flag_color_sequences;
	enum control_state control_state;
	wchar_t control_sequence[CONTROL_SEQUENCE_MAX];
	size_t control_sequence_length;
	bool input_set_color;
	struct line* lines;
	size_t lines_used;
	size_t lines_length;
	enum control_state incoming_control_state;
	struct line* incoming_line;
	size_t incoming_line_width;
	size_t current_line;
	size_t current_line_offset;
	bool allowance_ever_exhausted;
	bool skipping_to_end;
};

static void pager_init(struct pager* pager)
{
	pager->tty_fd = 0;
	if ( !isatty(pager->tty_fd) )
	{
		if ( (pager->tty_fd = open("/dev/tty", O_RDONLY)) < 0 )
			error(1, errno, "/dev/tty");
		if ( !isatty(pager->tty_fd) )
			error(1, errno, "/dev/tty");
	}
	gettermmode(pager->tty_fd, &pager->tty_fd_old_termmode);
	settermmode(pager->tty_fd, TERMMODE_KBKEY | TERMMODE_UNICODE | TERMMODE_SIGNAL);

	pager->out_fd = 1;
	if ( (pager->out_fd_is_tty = isatty(pager->out_fd)) )
	{
		tcgetwinsize(pager->out_fd, &pager->out_fd_winsize);
		pager->possible_lines = pager->out_fd_winsize.ws_row - 1;
		pager->allowed_lines = pager->possible_lines;
	}

	memset(&pager->in_ps, 0, sizeof(pager->in_ps));
	memset(&pager->out_ps, 0, sizeof(pager->out_ps));
}

static void pager_end(struct pager* pager)
{
	for ( size_t i = 0; i < pager->lines_used; i++ )
		free(pager->lines[i].content);
	free(pager->lines);
	settermmode(pager->tty_fd, pager->tty_fd_old_termmode);
	if ( pager->tty_fd != 0 )
		close(pager->tty_fd);
}

static void pager_prompt(struct pager* pager, bool at_end)
{
	const char* pre = pager->input_set_color ? "" : "\e[47;30m";
	const char* post = pager->input_set_color ? "" : "\e[m";
	if ( at_end )
		dprintf(pager->out_fd, "%s(END)%s\e[J", pre, post);
	else if ( pager->input_prompt_name[0] )
		dprintf(pager->out_fd, "%s%s%s\e[J", pre, pager->input_prompt_name, post);
	else
		dprintf(pager->out_fd, ":");
	uint32_t codepoint;
	while ( read(pager->tty_fd, &codepoint, sizeof(codepoint)) == sizeof(codepoint) )
	{
		if ( codepoint == L'\n' || KBKEY_DECODE(codepoint) == KBKEY_DOWN )
		{
			dprintf(pager->out_fd, "\r\e[J");
			pager->allowed_lines++;
			return;
		}

		if ( KBKEY_DECODE(codepoint) == KBKEY_UP )
		{
			if ( pager->current_line <= pager->possible_lines )
				continue;
			dprintf(pager->out_fd, "\e[2J\e[H");
			pager->current_line -= pager->possible_lines + 1;
			pager->current_line_offset = 0;
			pager->allowed_lines = pager->possible_lines;
			return;
		}

		if ( codepoint == L' ' || KBKEY_DECODE(codepoint) == KBKEY_PGDOWN )
		{
			dprintf(pager->out_fd, "\r\e[J");
			pager->allowed_lines = pager->possible_lines;
			return;
		}

		if ( KBKEY_DECODE(codepoint) == KBKEY_PGUP )
		{
			if ( pager->current_line <= pager->possible_lines )
				continue;
			size_t distance = pager->possible_lines;
			if ( pager->current_line - pager->possible_lines < distance )
				distance = pager->current_line - pager->possible_lines;
			dprintf(pager->out_fd, "\e[2J\e[H");
			pager->current_line -= pager->possible_lines + distance;
			pager->current_line_offset = 0;
			pager->allowed_lines = pager->possible_lines;
			return;
		}

		if ( KBKEY_DECODE(codepoint) == KBKEY_END )
		{
			dprintf(pager->out_fd, "\r\e[J");
			pager->skipping_to_end = true;
			pager->allowed_lines = SIZE_MAX;
			return;
		}

		if ( KBKEY_DECODE(codepoint) == KBKEY_HOME )
		{
			if ( pager->current_line <= pager->possible_lines )
				continue;
			dprintf(pager->out_fd, "\e[2J\e[H");
			pager->current_line = 0;
			pager->current_line_offset = 0;
			pager->allowed_lines = pager->possible_lines;
			return;
		}

		if ( codepoint == L'q' || codepoint == L'Q' )
		{
			dprintf(pager->out_fd, "\r\e[J");
			pager->quiting = true;
			return;
		}
	}
	error(0, errno, "/dev/tty");
	pager_end(pager);
	exit(1);
}

static void pager_line_push_char(struct pager* pager, struct line* line, char c)
{
	if ( line->content_used == line->content_length )
	{
		size_t new_length = 2 * line->content_length;
		if ( new_length == 0 )
			new_length = 128;
		char* new_content = (char*) realloc(line->content, new_length);
		if ( !new_content )
		{
			error(0, errno, "malloc");
			pager_end(pager);
			exit(1);
		}
		line->content = new_content;
		line->content_length = new_length;
	}
	line->content[line->content_used++] = c;
}

static struct line* pager_continue_line(struct pager* pager)
{
	if ( pager->incoming_line )
		return pager->incoming_line;
	if ( pager->lines_used == pager->lines_length )
	{
		size_t new_length = 2 * pager->lines_length;
		if ( new_length == 0 )
			new_length = 128;
		struct line* new_lines = (struct line*)
			reallocarray(pager->lines, new_length, sizeof(struct line));
		if ( !new_lines )
		{
			error(0, errno, "malloc");
			pager_end(pager);
			exit(1);
		}
		pager->lines = new_lines;
		pager->lines_length = new_length;
	}
	pager->incoming_line = &pager->lines[pager->lines_used++];
	memset(pager->incoming_line, 0, sizeof(*pager->incoming_line));
	pager->incoming_line_width = 0;
	return pager->incoming_line;
}

static void pager_finish_line(struct pager* pager)
{
	struct line* line = pager->incoming_line;
	assert(line);
	char* final_content = (char*) realloc(line->content, line->content_used);
	if ( final_content )
		line->content = final_content;
	pager->incoming_line = NULL;
	pager->incoming_line_width = 0;
}

static struct line* pager_next_line(struct pager* pager)
{
	pager_finish_line(pager);
	return pager_continue_line(pager);
}

static void pager_push_wchar(struct pager* pager, wchar_t wc)
{
	bool newline = false;
	int width;
	struct line* line = pager_continue_line(pager);

	if ( pager->incoming_control_state == CONTROL_STATE_CSI )
	{
		pager->incoming_control_state = CONTROL_STATE_NONE;
		if ( wc == '[' )
			pager->incoming_control_state = CONTROL_STATE_COMMAND;
	}
	else if ( pager->incoming_control_state == CONTROL_STATE_COMMAND )
	{
		pager->incoming_control_state = CONTROL_STATE_NONE;
		if ( ('0' <= wc && wc <= '9') ||
		     wc == L';' || wc == L':' || wc == L'?' )
			pager->incoming_control_state = CONTROL_STATE_COMMAND;
	}
	else if ( wc == L'\b' )
	{
		if ( pager->incoming_line_width )
			pager->incoming_line_width--;
	}
	else if ( wc == L'\e' )
	{
		pager->incoming_control_state = CONTROL_STATE_CSI;
	}
	else if ( wc == L'\n' )
	{
		newline = true;
	}
	else if ( wc == L'\t' )
	{
		if ( pager->out_fd_winsize.ws_col == pager->incoming_line_width )
			line = pager_next_line(pager);
		do
		{
			if ( pager->out_fd_winsize.ws_col == pager->incoming_line_width )
				break;
			pager->incoming_line_width++;
		} while ( pager->incoming_line_width % 8 != 0 );
	}
	else if ( wc == L'\r' )
	{
		pager->incoming_line_width = 0;
	}
	else if ( wc == 127 )
	{
	}
	else if ( 0 <= (width = wcwidth(wc)) )
	{
		size_t left = pager->out_fd_winsize.ws_col - pager->incoming_line_width;
		if ( left < (size_t) width )
			line = pager_next_line(pager);
		pager->incoming_line_width += width;
	}
	else
	{
		// TODO: What can cause this and how to handle it?
	}

	char mb[MB_CUR_MAX];
	size_t amount = wcrtomb(mb, wc, &pager->out_ps);
	if ( amount != (size_t) -1 )
	{
		for ( size_t i = 0; i < amount; i++ )
			pager_line_push_char(pager, line, mb[i]);
	}

	if ( newline )
		pager_finish_line(pager);
}

static bool pager_push_wchar_is_escaped(struct pager* pager, wchar_t wc)
{
	if ( wc == '\b' &&
	    (pager->flag_raw_control_chars || pager->flag_color_sequences) )
		return false;
	return wc < 32 && wc != L'\t' && wc != L'\n';
}

static void pager_push_wchar_escape(struct pager* pager, wchar_t wc)
{
	if ( pager_push_wchar_is_escaped(pager, wc) )
	{
		pager_push_wchar(pager, L'^');
		//pager_push_wchar(pager, L'\b');
		//pager_push_wchar(pager, L'^');
		pager_push_wchar(pager, L'@' + wc);
		//pager_push_wchar(pager, L'\b');
		//pager_push_wchar(pager, L'@' + wc);
	}
	else
	{
		pager_push_wchar(pager, wc);
	}
}

static void pager_control_sequence_begin(struct pager* pager)
{
	pager->control_sequence_length = 0;
}

static void pager_control_sequence_accept(struct pager* pager)
{
	for ( size_t i = 0; i < pager->control_sequence_length; i++ )
		pager_push_wchar(pager, pager->control_sequence[i]);
	pager->control_sequence_length = 0;
	pager->control_state = CONTROL_STATE_NONE;
}

static void pager_control_sequence_reject(struct pager* pager)
{
	for ( size_t i = 0; i < pager->control_sequence_length; i++ )
		pager_push_wchar_escape(pager, pager->control_sequence[i]);
	pager->control_sequence_length = 0;
	pager->control_state = CONTROL_STATE_NONE;
}

static void pager_control_sequence_push(struct pager* pager, wchar_t wc)
{
	if ( pager->flag_raw_control_chars )
		return pager_push_wchar(pager, wc);
	if ( CONTROL_SEQUENCE_MAX <= pager->control_sequence_length )
	{
		pager_control_sequence_reject(pager);
		pager_push_wchar_escape(pager, wc);
		return;
	}
	pager->control_sequence[pager->control_sequence_length++] = wc;
}

static void pager_control_sequence_finish(struct pager* pager, wchar_t wc)
{
	pager_control_sequence_push(pager, wc);
	if ( pager->control_state == CONTROL_STATE_NONE )
		return;
	if ( wc == L'm' )
	{
		pager->input_set_color = true;
		return pager_control_sequence_accept(pager);
	}
	if ( !pager->flag_raw_control_chars )
		return pager_control_sequence_reject(pager);
	pager_control_sequence_accept(pager);
}

static void pager_push_wchar_filter(struct pager* pager, wchar_t wc)
{
	if ( wc == L'\e' &&
	     (pager->flag_raw_control_chars || pager->flag_color_sequences) &&
	     pager->control_state == CONTROL_STATE_NONE )
	{
		pager_control_sequence_begin(pager);
		pager_control_sequence_push(pager, wc);
		pager->control_state = CONTROL_STATE_CSI;
		return;
	}
	else if ( pager->control_state == CONTROL_STATE_CSI )
	{
		if ( wc == L'[' )
		{
			pager_control_sequence_push(pager, wc);
			pager->control_state = CONTROL_STATE_COMMAND;
			return;
		}
		pager_control_sequence_reject(pager);
	}
	else if ( pager->control_state == CONTROL_STATE_COMMAND )
	{
		if ( ('0' <= wc && wc <= '9') ||
		     wc == L';' || wc == L':' || wc == L'?' )
		{
			pager_control_sequence_push(pager, wc);
			return;
		}
		pager_control_sequence_finish(pager, wc);
		return;
	}
	pager_push_wchar_escape(pager, wc);
}

static void pager_push_byte(struct pager* pager, unsigned char byte)
{
	if ( pager->quiting )
		return;

	wchar_t wc;
	size_t amount = mbrtowc(&wc, (const char*) &byte, 1, &pager->in_ps);
	if ( amount == (size_t) -2 )
		return;
	if ( amount == (size_t) -1 )
	{
		wc = 0xFFFD /* REPLACEMENT CHARACTER */;
		memset(&pager->in_ps, 0, sizeof(pager->in_ps));
	}
	pager_push_wchar_filter(pager, wc);
}

static bool pager_read_fd(struct pager* pager, int fd, const char* fdpath)
{
	unsigned char buffer[4096];
	ssize_t amount = read(fd, buffer, sizeof(buffer));
	if ( amount < 0 )
	{
		error(0, errno, "%s", fdpath);
		pager_end(pager);
		exit(1);
	}
	for ( ssize_t i = 0; i < amount; i++ )
		pager_push_byte(pager, buffer[i]);
	return amount != 0;
}

static void pager_simple_fd(struct pager* pager, int fd, const char* fdpath)
{
	unsigned char buffer[4096];
	ssize_t amount = 0;
	while ( 0 < (amount = read(fd, buffer, sizeof(buffer))) )
	{
		if ( writeall(pager->out_fd, buffer, amount) < (size_t) amount )
		{
			error(0, errno, "<stdout>");
			pager_end(pager);
			exit(1);
		}
	}
	if ( amount < 0 )
	{
		error(0, errno, "%s", fdpath);
		pager_end(pager);
		exit(1);
	}
}

static bool pager_can_page(struct pager* pager)
{
	if ( pager->current_line + 1 == pager->lines_used )
	{
		struct line* line = &pager->lines[pager->current_line];
		return pager->current_line_offset < line->content_used;
	}
	return pager->current_line + 1 < pager->lines_used;
}

static void pager_page(struct pager* pager)
{
	struct line* line = &pager->lines[pager->current_line];
	if ( pager->current_line_offset < line->content_used )
	{
		const char* buffer = line->content + pager->current_line_offset;
		size_t amount = line->content_used - pager->current_line_offset;
		if ( writeall(pager->out_fd, buffer, amount) < amount )
		{
			error(0, errno, "write: <stdout>");
			pager_end(pager);
			exit(1);
		}
		pager->current_line_offset = line->content_used;
	}
	if ( pager->current_line + 1 < pager->lines_used )
	{
		if ( pager->allowed_lines != SIZE_MAX )
			pager->allowed_lines--;
		pager->current_line++;
		pager->current_line_offset = 0;
	}
}

static void pager_push_fd(struct pager* pager, int fd, const char* fdpath)
{
	if ( pager->quiting )
		return;
	if ( !strcmp(fdpath, "<stdin>") )
		pager->input_prompt_name = "";
	else
		pager->input_prompt_name = fdpath;
	// TODO: In this case, we should disable echoing and read from the terminal
	//       anyways. Remember to enable it again.
	if ( isatty(fd) )
	{
		error(0, 0, "/dev/tty: Is a terminal");
		pager_end(pager);
		exit(1);
	}
	if ( !pager->out_fd_is_tty )
		return pager_simple_fd(pager, fd, fdpath);
	bool eof = false;
	while ( !pager->quiting )
	{
		if ( !pager->skipping_to_end )
		{
			if ( pager->allowed_lines == 0 )
			{
				pager->allowance_ever_exhausted = true;
				pager_prompt(pager, false);
				continue;
			}
			if ( pager_can_page(pager) )
			{
				pager_page(pager);
				continue;
			}
		}
		if ( eof )
			break;
		if ( !pager_read_fd(pager, fd, fdpath) )
			eof = true;
	}
}

static void pager_push_path(struct pager* pager, const char* path)
{
	if ( pager->quiting )
		return;
	if ( !strcmp(path, "-") )
		return pager_push_fd(pager, 0, "<stdin>");
	int fd = open(path, O_RDONLY);
	if ( fd < 0 )
	{
		error(0, errno, "%s", path);
		pager_end(pager);
		exit(1);
	}
	pager_push_fd(pager, fd, path);
	close(fd);
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
	fprintf(fp, "Usage: %s [OPTION]... [FILES]...\n", argv0);
	fprintf(fp, "Displays files one page at a time.\n");
}

static void version(FILE* fp, const char* argv0)
{
	fprintf(fp, "%s (Sortix) %s\n", argv0, VERSIONSTR);
}

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "");

	struct pager pager;
	memset(&pager, 0, sizeof(pager));

	if ( getenv("LESS") )
	{
		const char* options = getenv("LESS");
		char c;
		while ( (c = *options++) )
		{
			switch ( c )
			{
			case '-': break;
			case 'r': pager.flag_raw_control_chars = true; break;
			case 'R': pager.flag_color_sequences = true; break;
			default: break;
			}
		}
	}

	const char* argv0 = argv[0];
	for ( int i = 1; i < argc; i++ )
	{
		const char* arg = argv[i];
		if ( arg[0] != '-' || !arg[1] )
			continue;
		argv[i] = NULL;
		if ( !strcmp(arg, "--") )
			break;
		if ( arg[1] != '-' )
		{
			char c;
			while ( (c = *++arg) ) switch ( c )
			{
			case 'r': pager.flag_raw_control_chars = true; break;
			case 'R': pager.flag_color_sequences = true; break;
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
		else
		{
			fprintf(stderr, "%s: unknown option: %s\n", argv0, arg);
			help(stderr, argv0);
			exit(1);
		}
	}

	compact_arguments(&argc, &argv);

	pager_init(&pager);

	if ( argc == 1 )
	{
		if ( pager.tty_fd == 0 )
		{
			error(0, 0, "missing file operand");
			pager_end(&pager);
			exit(1);
		}
		pager_push_fd(&pager, 0, "<stdin>");
	}
	else for ( int i = 1; i < argc; i++ )
	{
		pager_push_path(&pager, argv[i]);
	}

	while ( pager.out_fd_is_tty &&
	        pager.allowance_ever_exhausted &&
	        !pager.quiting )
	{
		if ( pager.skipping_to_end )
		{
			dprintf(pager.out_fd, "\e[2J\e[H");
			size_t line = 0;
			if ( pager.possible_lines <= pager.lines_used )
				line = pager.lines_used - pager.possible_lines;
			pager.current_line = line;
			pager.current_line_offset = 0;
			pager.allowed_lines = pager.possible_lines;
			pager.skipping_to_end = false;
		}
		bool cant_page = !pager_can_page(&pager);
		if ( cant_page || pager.allowed_lines == 0 )
		{
			pager_prompt(&pager, cant_page);
			continue;
		}
		pager_page(&pager);
	}

	pager_end(&pager);

	return 0;
}
