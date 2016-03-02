/*
 * Copyright (c) 2013, 2015 Jonas 'Sortie' Termansen.
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
 * command-not-found.c
 * Prints a notice that the attempted command wasn't found and possibly
 * suggests how to install the command or suggests the proper spelling in case
 * of a typo.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void suggest_editor(const char* filename)
{
	fprintf(stderr, "No command '%s' found, did you mean:\n", filename);
	fprintf(stderr, " Command 'editor' from package 'utils'\n");
}

void suggest_pager(const char* filename)
{
	fprintf(stderr, "No command '%s' found, did you mean:\n", filename);
	fprintf(stderr, " Command 'pager' from package 'utils'\n");
}

void suggest_extfs(const char* filename)
{
	fprintf(stderr, "No command '%s' found, did you mean:\n", filename);
	fprintf(stderr, " Command 'extfs' from package 'ext'\n");
}

void suggest_unmount(const char* filename)
{
	fprintf(stderr, "No command '%s' found, did you mean:\n", filename);
	fprintf(stderr, " Command 'unmount' from package 'utils'\n");
}

void suggest_logout(const char* filename)
{
	fprintf(stderr, "No command '%s' found, did you mean:\n", filename);
	fprintf(stderr, " Exiting your shell normally to logout.\n");
}

void suggest_poweroff(const char* filename)
{
	fprintf(stderr, "No command '%s' found, did you mean:\n", filename);
	if ( getenv("LOGIN_PID") )
	{
		fprintf(stderr, " Exiting your shell normally to logout.\n");
		fprintf(stderr, " Login as user 'poweroff' to power off computer.\n");
	}
	else
	{
		fprintf(stderr, " Exiting your shell normally to poweroff.\n");
	}
}

void suggest_reboot(const char* filename)
{
	fprintf(stderr, "No command '%s' found, did you mean:\n", filename);
	if ( getenv("LOGIN_PID") )
	{
		fprintf(stderr, " Exiting your shell normally to logout.\n");
		fprintf(stderr, " Login as user 'reboot' to reboot computer.\n");
	}
	else
	{
		fprintf(stderr, " Exiting your shell with 'exit 1' to reboot.\n");
	}
}

int main(int argc, char* argv[])
{
	const char* filename = 2 <= argc ? argv[1] : argv[0];
	if ( !strcmp(filename, "ed") ||
	     !strcmp(filename, "emacs") ||
	     !strcmp(filename, "nano") ||
	     !strcmp(filename, "vi") ||
	     !strcmp(filename, "vim") )
		suggest_editor(filename);
	else if ( !strcmp(filename, "less") ||
	          !strcmp(filename, "more") )
		suggest_pager(filename);
	else if ( !strcmp(filename, "mount") )
		suggest_extfs(filename);
	else if ( !strcmp(filename, "umount") )
		suggest_unmount(filename);
	else if ( !strcmp(filename, "logout") ||
	          !strcmp(filename, "logoff") )
		suggest_logout(filename);
	else if ( !strcmp(filename, "poweroff") ||
	          !strcmp(filename, "halt") ||
	          !strcmp(filename, "shutdown") )
		suggest_poweroff(filename);
	else if ( !strcmp(filename, "reboot") )
		suggest_reboot(filename);
	fprintf(stderr, "%s: command not found\n", filename);
	return 127;
}
