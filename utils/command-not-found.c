/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013, 2015.

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

    command-not-found.c
    Prints a notice that the attempted command wasn't found and possibly
    suggests how to install the command or suggests the proper spelling in case
    of a typo.

*******************************************************************************/

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
