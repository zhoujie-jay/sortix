/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2014.

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

#if !defined(VERSIONSTR)
#define VERSIONSTR "unknown version"
#endif

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
			error(1, errno, "malloc");
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
	if ( pager->input_prompt_name[0] )
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
	error(1, errno, "/dev/tty");
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
		error(1, errno, "write: <stdout>");
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
	if ( wc < 32 && wc != L'\t' && wc != L'\n' )
	{
		dprintf(pager->out_fd, "\e[97m");
		pager_push_char(pager, L'^');
		pager_push_char(pager, L'@' + wc);
		dprintf(pager->out_fd, "\e[m");
	}
	else
	{
		pager_push_final_char(pager, wc);
	}
}

void pager_push_byte(struct pager* pager, unsigned char byte)
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
	pager_push_char(pager, wc);
}

void pager_simple_output_fd(struct pager* pager, int fd, const char* fdpath)
{
	unsigned char buffer[4096];
	ssize_t in_amount;
	while ( 0 < (in_amount = read(fd, buffer, sizeof(buffer))) )
	{
		if ( writeall(pager->out_fd, buffer, in_amount) < (size_t) in_amount )
			error(1, errno, "<stdout>");
	}
	if ( in_amount < 0 )
		error(1, errno, "%s", fdpath);
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
		error(1, errno, "/dev/tty");
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
		error(1, errno, "%s", fdpath);
}

void pager_push_path(struct pager* pager, const char* path)
{
	if ( pager->quiting )
		return;

	if ( !strcmp(path, "-") )
		return pager_push_fd(pager, 0, "<stdin>");
	int fd = open(path, O_RDONLY);
	if ( fd < 0 )
		error(1, errno, "%s", path);
	pager_push_fd(pager, fd, path);
	close(fd);
}

void compact_arguments(int* argc, char*** argv)
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

void help(FILE* fp, const char* argv0)
{
	fprintf(fp, "Usage: %s [OPTION]... [FILES]...\n", argv0);
	fprintf(fp, "Displays files one page at a time.\n");
}

void version(FILE* fp, const char* argv0)
{
	fprintf(fp, "%s (Sortix) %s\n", argv0, VERSIONSTR);
	fprintf(fp, "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n");
	fprintf(fp, "This is free software: you are free to change and redistribute it.\n");
	fprintf(fp, "There is NO WARRANTY, to the extent permitted by law.\n");
}

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "");

	const char* argv0 = argv[0];
	for ( int i = 1; i < argc; i++ )
	{
		const char* arg = argv[i];
		if ( arg[0] != '-' )
			continue;
		argv[i] = NULL;
		if ( !strcmp(arg, "--") )
			break;
		if ( arg[1] != '-' )
		{
			while ( char c = *++arg ) switch ( c )
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
		else
		{
			fprintf(stderr, "%s: unknown option: %s\n", argv0, arg);
			help(stderr, argv0);
			exit(1);
		}
	}

	compact_arguments(&argc, &argv);

	struct pager pager;
	memset(&pager, 0, sizeof(pager));
	pager_init(&pager);

	if ( argc == 1 )
	{
		if ( pager.tty_fd == 0 )
			error(1, 0, "missing file operand");
		pager_push_fd(&pager, 0, "<stdin>");
	}
	else for ( int i = 1; i < argc; i++ )
	{
		pager_push_path(&pager, argv[i]);
	}

	return 0;
}
