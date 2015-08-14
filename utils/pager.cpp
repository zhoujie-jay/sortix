/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2014, 2015.

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

    pager.cpp
    Displays files one page at a time.

*******************************************************************************/

#include <sys/keycodes.h>
#include <sys/termmode.h>

#include <assert.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <ioleast.h>
#include <locale.h>
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

struct display_entry
{
	wchar_t c;
};

struct pager
{
	int tty_fd;
	unsigned int tty_fd_old_termmode;
	int out_fd;
	bool out_fd_tty;
	struct winsize out_fd_winsize;
	struct display_entry* display;
	struct wincurpos begun_wcp;
	struct wincurpos current_wcp;
	mbstate_t in_ps;
	mbstate_t out_ps;
	const char* input_prompt_name;
	size_t allowed_lines;
	bool quiting;
	bool flag_raw_control_chars;
	bool flag_color_sequences;
	enum control_state control_state;
	unsigned char control_sequence[CONTROL_SEQUENCE_MAX];
	size_t control_sequence_length;
	bool input_set_color;
};

void pager_init(struct pager* pager)
{
	if ( !isatty(pager->tty_fd = 0) )
	{
		if ( (pager->tty_fd = open("/dev/tty", O_RDONLY)) < 0 )
			error(1, errno, "/dev/tty");
		if ( !isatty(pager->tty_fd) )
			error(1, errno, "/dev/tty");
	}
	gettermmode(pager->tty_fd, &pager->tty_fd_old_termmode);
	settermmode(pager->tty_fd, TERMMODE_KBKEY | TERMMODE_UNICODE | TERMMODE_SIGNAL);

	if ( (pager->out_fd_tty = isatty(pager->out_fd = 1)) )
	{
		tcgetwinsize(pager->out_fd, &pager->out_fd_winsize);
		size_t display_length = (size_t) pager->out_fd_winsize.ws_col *
		                        (size_t) pager->out_fd_winsize.ws_row;
		pager->display = (struct display_entry*)
			calloc(display_length, sizeof(struct display_entry));
		if ( !pager->display )
		{
			error(0, errno, "malloc");
			settermmode(pager->tty_fd, pager->tty_fd_old_termmode);
			exit(1);
		}
		tcgetwincurpos(pager->out_fd, &pager->begun_wcp);
		memcpy(&pager->current_wcp, &pager->begun_wcp, sizeof(struct wincurpos));
		pager->allowed_lines = pager->out_fd_winsize.ws_row - 1;
	}

	memset(&pager->in_ps, 0, sizeof(pager->in_ps));
	memset(&pager->out_ps, 0, sizeof(pager->out_ps));
}

void pager_end(struct pager* pager)
{
	settermmode(pager->tty_fd, pager->tty_fd_old_termmode);
	if ( pager->tty_fd != 0 )
		close(pager->tty_fd);
}

void pager_prompt(struct pager* pager)
{
	if ( pager->input_prompt_name[0] && pager->input_set_color )
		dprintf(pager->out_fd, "%s\e[J", pager->input_prompt_name);
	else if ( pager->input_prompt_name[0] && !pager->input_set_color )
		dprintf(pager->out_fd, "\e[47;30m%s\e[m\e[J", pager->input_prompt_name);
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

		if ( codepoint == L' ' || KBKEY_DECODE(codepoint) == KBKEY_PGDOWN )
		{
			dprintf(pager->out_fd, "\r\e[J");
			pager->allowed_lines = pager->out_fd_winsize.ws_row - 1;
			return;
		}

		if ( KBKEY_DECODE(codepoint) == KBKEY_END )
		{
			dprintf(pager->out_fd, "\r\e[J");
			pager->allowed_lines = SIZE_MAX;
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

bool pager_would_push_newline_scroll(struct pager* pager)
{
	return pager->out_fd_winsize.ws_row <= pager->current_wcp.wcp_row + 1;
}

bool pager_would_push_char_overflow(struct pager* pager, wchar_t wc)
{
	if ( wc == L'\r' )
		return false;
	return pager->out_fd_winsize.ws_col <= pager->current_wcp.wcp_col;
}

bool pager_would_push_char_scroll(struct pager* pager, wchar_t wc)
{
	if ( wc == L'\n' )
		return pager_would_push_newline_scroll(pager);
	if ( pager_would_push_char_overflow(pager, wc) )
		return pager_would_push_newline_scroll(pager);
	return false;
}

void pager_push_newline(struct pager* pager)
{
	if ( 0 < pager->allowed_lines && pager->allowed_lines != SIZE_MAX )
		pager->allowed_lines--;
	if ( pager->current_wcp.wcp_row + 1 < pager->out_fd_winsize.ws_row )
	{
		pager->current_wcp.wcp_row++;
	}
	else
	{
		if ( 0 < pager->begun_wcp.wcp_row )
			pager->begun_wcp.wcp_row--;
	}
	pager->current_wcp.wcp_col = 0;
}

bool pager_is_out_of_allowed_lines(struct pager* pager, wchar_t wc)
{
	if ( pager->allowed_lines == 1 && wc != L'\n' &&
	     pager_would_push_char_overflow(pager, wc) )
		return true;
	return pager->allowed_lines == 0;
}

void pager_push_final_char(struct pager* pager, wchar_t wc)
{
	if ( pager->quiting || !wc )
		return;

	while ( pager_is_out_of_allowed_lines(pager, wc) )
	{
		pager_prompt(pager);
		if ( pager->quiting )
			return;
	}

	mbstate_t ps;
	memcpy(&ps, &pager->out_ps, sizeof(mbstate_t));
	char mb[MB_CUR_MAX];
	size_t mb_size = wcrtomb(mb, wc, &ps);
	if ( mb_size == (size_t) -1 )
	{
		memset(&pager->out_ps, 0, sizeof(pager->out_ps));
		return;
	}

	if ( writeall(pager->out_fd, mb, mb_size) < mb_size )
	{
		error(0, errno, "write: <stdout>");
		pager_end(pager);
		exit(1);
	}
	memcpy(&pager->out_ps, &ps, sizeof(mbstate_t));

	if ( wc == L'\n' )
	{
		pager_push_newline(pager);
	}
	else if ( wc == L'\t' )
	{
		if ( pager_would_push_char_overflow(pager, ' ') )
			pager_push_newline(pager);
		do
		{
			if ( pager_would_push_char_overflow(pager, ' ') )
				break;
			pager->current_wcp.wcp_col++;
		} while ( pager->current_wcp.wcp_col % 8 != 0 );
	}
	else if ( wc == L'\r' )
	{
		pager->current_wcp.wcp_col = 0;
	}
	else
	{
		if ( pager_would_push_char_overflow(pager, wc) )
			pager_push_newline(pager);
		pager->current_wcp.wcp_col++;
	}
}

void pager_push_char(struct pager* pager, wchar_t wc)
{
	if ( wc == L'\b' &&
	     (pager->flag_raw_control_chars || pager->flag_color_sequences) )
	{
		dprintf(pager->out_fd, "\b");
		tcgetwincurpos(pager->out_fd, &pager->current_wcp);
	}
	else if ( wc < 32 && wc != L'\t' && wc != L'\n' )
	{
		if ( !pager->input_set_color )
			dprintf(pager->out_fd, "\e[97m");
		pager_push_final_char(pager, L'^');
		pager_push_final_char(pager, L'@' + wc);
		if ( !pager->input_set_color )
			dprintf(pager->out_fd, "\e[m");
	}
	else
	{
		pager_push_final_char(pager, wc);
	}
}

void pager_control_sequence_begin(struct pager* pager)
{
	pager->control_sequence_length = 0;
}

void pager_control_sequence_accept(struct pager* pager)
{
	unsigned char* cs = pager->control_sequence;
	size_t cs_length = pager->control_sequence_length;
	if ( writeall(pager->out_fd, cs, cs_length) < cs_length )
	{
		error(0, errno, "write: <stdout>");
		pager_end(pager);
		exit(1);
	}
	pager->control_sequence_length = 0;
	pager->control_state = CONTROL_STATE_NONE;
}

void pager_control_sequence_reject(struct pager* pager)
{
	for ( size_t i = 0; i < pager->control_sequence_length; i++ )
	{
		wint_t wc = btowc(pager->control_sequence[i]);
		if ( wc != WEOF )
			pager_push_char(pager, wc);
	}
	pager->control_sequence_length = 0;
	pager->control_state = CONTROL_STATE_NONE;
}

void pager_control_sequence_push(struct pager* pager, unsigned char byte)
{
	if ( CONTROL_SEQUENCE_MAX <= pager->control_sequence_length )
	{
		if ( !pager->flag_raw_control_chars )
			return pager_control_sequence_reject(pager);
		unsigned char* cs = pager->control_sequence;
		size_t cs_length = pager->control_sequence_length;
		if ( writeall(pager->out_fd, cs, cs_length) < cs_length )
		{
			error(0, errno, "write: <stdout>");
			pager_end(pager);
			exit(1);
		}
		pager->control_sequence_length = 0;
	}
	pager->control_sequence[pager->control_sequence_length++] = byte;
}

void pager_control_sequence_finish(struct pager* pager, unsigned char byte)
{
	pager_control_sequence_push(pager, byte);
	if ( pager->control_state == CONTROL_STATE_NONE )
		return;
	if ( byte == 'm' )
	{
		pager->input_set_color = true;
		return pager_control_sequence_accept(pager);
	}
	if ( !pager->flag_raw_control_chars )
		return pager_control_sequence_reject(pager);
	pager_control_sequence_accept(pager);
	tcgetwincurpos(pager->out_fd, &pager->current_wcp);
}

void pager_push_byte(struct pager* pager, unsigned char byte)
{
	if ( pager->quiting )
		return;

	if ( byte == '\e' && mbsinit(&pager->in_ps) &&
	     (pager->flag_raw_control_chars || pager->flag_color_sequences) &&
	     pager->control_state == CONTROL_STATE_NONE )
	{
		pager_control_sequence_begin(pager);
		pager_control_sequence_push(pager, byte);
		pager->control_state = CONTROL_STATE_CSI;
		return;
	}
	else if ( pager->control_state == CONTROL_STATE_CSI )
	{
		if ( byte == '[' )
		{
			pager_control_sequence_push(pager, byte);
			pager->control_state = CONTROL_STATE_COMMAND;
			return;
		}
		pager_control_sequence_reject(pager);
	}
	else if ( pager->control_state == CONTROL_STATE_COMMAND )
	{
		if ( ('0' <= byte && byte <= '9') ||
		     byte == ';' || byte == ':' || byte == '?' )
		{
			pager_control_sequence_push(pager, byte);
			return;
		}
		pager_control_sequence_finish(pager, byte);
		return;
	}

	wchar_t wc;
	size_t amount = mbrtowc(&wc, (const char*) &byte, 1, &pager->in_ps);
	if ( amount == (size_t) -2 )
		return;
	if ( amount == (size_t) -1 )
	{
		wc = 0xFFFD /* REPLACEMENT CHARACTER */;
		memset(&pager->in_ps, 0, sizeof(pager->in_ps));
	}
	pager_push_char(pager, wc);
}

void pager_simple_output_fd(struct pager* pager, int fd, const char* fdpath)
{
	unsigned char buffer[4096];
	ssize_t in_amount;
	while ( 0 < (in_amount = read(fd, buffer, sizeof(buffer))) )
	{
		if ( writeall(pager->out_fd, buffer, in_amount) < (size_t) in_amount )
		{
			error(0, errno, "<stdout>");
			pager_end(pager);
			exit(1);
		}
	}
	if ( in_amount < 0 )
	{
		error(0, errno, "%s", fdpath);
		pager_end(pager);
		exit(1);
	}
}

void pager_push_fd(struct pager* pager, int fd, const char* fdpath)
{
	if ( pager->quiting )
		return;

	if ( !strcmp(fdpath, "<stdin>") )
		pager->input_prompt_name = "";
	else
		pager->input_prompt_name = fdpath;
	// TODO: In this case, we should disable echoing and read from the terminal
	//       anyways.
	if ( isatty(fd) )
	{
		error(0, errno, "/dev/tty");
		pager_end(pager);
		exit(1);
	}
	if ( !pager->out_fd_tty )
		return pager_simple_output_fd(pager, fd, fdpath);
	unsigned char buffer[4096];
	ssize_t in_amount = 0;
	while ( !pager->quiting &&
	        0 < (in_amount = read(fd, buffer, sizeof(buffer))) )
	{
		for ( ssize_t i = 0; i < in_amount; i++ )
			pager_push_byte(pager, buffer[i]);
	}
	if ( in_amount < 0 )
	{
		error(0, errno, "%s", fdpath);
		pager_end(pager);
		exit(1);
	}
}

void pager_push_path(struct pager* pager, const char* path)
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
	fprintf(fp, "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n");
	fprintf(fp, "This is free software: you are free to change and redistribute it.\n");
	fprintf(fp, "There is NO WARRANTY, to the extent permitted by law.\n");
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
			while ( char c = *++arg ) switch ( c )
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

	pager_end(&pager);

	return 0;
}
