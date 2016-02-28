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

    ps.c
    Lists processes.

*******************************************************************************/

#include <sys/types.h>

#include <err.h>
#include <errno.h>
#include <locale.h>
#include <psctl.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

	bool select_all = false;
	bool show_full = false;
	bool show_long = false;

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
			case 'a': select_all = true; break;
			case 'A': select_all = true; break;
			case 'd': select_all = true; break;
			case 'e': select_all = true; break;
			case 'f': show_full = true; break;
			//case 'g': break;
			//case 'G': break;
			case 'l': show_long = true; break;
			//case 'n': break;
			//case 'o': break;
			//case 'p': break;
			//case 't': break;
			//case 'u': break;
			//case 'U': break;
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

	if ( show_full || show_long )
		printf("UID\t");
	printf("PID\t");
	if ( show_full || show_long )
		printf("PPID\t");
	if ( show_long )
		printf("NI  ");
	printf("TTY  ");
	printf("TIME\t   ");
	printf("CMD\n");
	pid_t pid = 0;
	while ( true )
	{
		struct psctl_next_pid ctl_next_pid;
		if ( psctl(pid, PSCTL_NEXT_PID, &ctl_next_pid) < 0 )
			err(1, "psctl: PSCTL_NEXT_PID");
		if ( (pid = ctl_next_pid.next_pid) == -1 )
			break;
		struct psctl_stat psst;
		if ( psctl(pid, PSCTL_STAT, &psst) < 0 )
		{
			if ( errno != ESRCH )
				warn("psctl: PSCTL_STAT: [%" PRIiPID "]", pid);
			continue;
		}
		if ( !select_all && psst.euid != geteuid() )
			continue;
		if ( show_full )
		{
			struct passwd* pwd = getpwuid(psst.uid);
			if ( pwd )
				printf("%s\t", pwd->pw_name);
			else
				printf("%" PRIuUID "\t", psst.uid);
		}
		else if ( show_long )
			printf("%" PRIuUID "\t", psst.uid);
		printf("%" PRIiPID "\t", pid);
		if ( show_full || show_long )
			printf("%" PRIiPID "\t", psst.ppid);
		if ( show_long )
			printf("%-4i", psst.nice);
		printf("tty  ");
		time_t time = psst.tmns.tmns_utime.tv_sec;
		int hours = (time / (60 * 60)) % 24;
		int minutes = (time / 60) % 60;
		int seconds = (time / 1) % 60;
		printf("%02i:%02i:%02i  ", hours, minutes, seconds);
		char* program_path = get_program_path_of_pid(pid);
		// TODO: Strip special characters from the process name lest an attacker
		//       do things to the user's terminal.
		printf("%s", program_path ? program_path : "<unknown>");
		free(program_path);
		printf("\n");
	}

	return ferror(stdout) || fflush(stdout) == EOF ? 1 : 0;
}
