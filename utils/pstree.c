/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2015.

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

    pstree.c
    Lists processes in a nice tree.

*******************************************************************************/

#include <sys/types.h>

#include <err.h>
#include <errno.h>
#include <locale.h>
#include <psctl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char* last_basename(const char* path)
{
	const char* result = path;
	for ( size_t i = 0; path[i]; i++ )
		if ( path[i] == '/' && path[i + 1] != '/' && path[i + 1] )
			result = path + i + 1;
	return result;
}

static char* get_program_path_of_pid(pid_t pid)
{
	struct psctl_program_path ctl;
	memset(&ctl, 0, sizeof(ctl));
	ctl.buffer = NULL;
	ctl.size = 0;
	if ( psctl(pid, PSCTL_PROGRAM_PATH, &ctl) < 0 )
		return NULL;
	while ( true )
	{
		char* new_buffer = (char*) realloc(ctl.buffer, ctl.size);
		if ( !new_buffer )
			return free(ctl.buffer), (char*) NULL;
		ctl.buffer = new_buffer;
		if ( psctl(pid, PSCTL_PROGRAM_PATH, &ctl) == 0 )
			return ctl.buffer;
		if ( errno != ERANGE )
			return free(ctl.buffer), (char*) NULL;
	}
}

static void pstree(pid_t pid, const char* prefix, bool continuation)
{
	while ( pid != -1 )
	{
		struct psctl_stat psst;
		if ( psctl(pid, PSCTL_STAT, &psst) < 0 )
		{
			if ( errno != ESRCH )
				warn("psctl: PSCTL_STAT: [%" PRIiPID "]", pid);
			return;
		}
		char* path_string = get_program_path_of_pid(pid);
		const char* path = last_basename(path_string ? path_string : "<unknown>");
		if ( !continuation )
			fputs(prefix, stdout);
		if ( prefix[0] )
		{
			if ( continuation )
				fputs("─", stdout);
			else
				fputs(" ", stdout);
			if ( continuation )
				fputs(psst.ppid_next == -1 ? "─" : "┬", stdout);
			else
				fputs(psst.ppid_next == -1 ? "└" : "│", stdout);
			fputs("─", stdout);
		}
		printf("%s", path);
		if ( psst.ppid_first != -1 )
		{
			size_t path_length = strlen(path);
			char* new_prefix;
			if ( prefix[0] )
			{
				size_t prefix_length = strlen(prefix);
				size_t new_prefix_length = prefix_length + 3 + path_length;
				if ( !(new_prefix = (char*) malloc(new_prefix_length + 1)) )
					err(1, "malloc");
				memcpy(new_prefix, prefix, prefix_length);
				memcpy(new_prefix + prefix_length,
					   psst.ppid_next != -1 ? " | " : "   ", 3);
				for ( size_t i = 0; i < path_length; i++ )
					new_prefix[prefix_length + 3 + i] = ' ';
				new_prefix[prefix_length + 3 + path_length] = '\0';
			}
			else
			{
				if ( !(new_prefix = (char*) malloc(path_length + 1)) )
					err(1, "malloc");
				for ( size_t i = 0; i < path_length; i++ )
					new_prefix[i] = ' ';
				new_prefix[path_length] = '\0';
			}
			pstree(psst.ppid_first, new_prefix, true);
			free(new_prefix);
		}
		else
			printf("\n");
		free(path_string);
		continuation = false;
		pid = psst.ppid_next;
	}
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
	fprintf(fp, "Usage: %s [OPTION]...\n", argv0);
	fprintf(fp, "List processes.\n");
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

	if ( 1 < argc )
		err(1, "extra operand: %s", argv[1]);

	pstree(1, "", true);

	return ferror(stdout) || fflush(stdout) == EOF ? 1 : 0;
}
