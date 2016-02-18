/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2016.

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

    sysmerge.c
    Upgrade current operating system from a sysroot.

*******************************************************************************/

#include <sys/types.h>

#include <err.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "conf.h"
#include "execute.h"
#include "fileops.h"
#include "manifest.h"
#include "release.h"

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

static bool has_pending_upgrade(void)
{
	return access_or_die("/boot/sortix.bin.sysmerge.orig", F_OK) == 0 ||
	       access_or_die("/boot/sortix.initrd.sysmerge.orig", F_OK) == 0 ||
	       access_or_die("/sysmerge", F_OK) == 0;
}

static void help(FILE* fp, const char* argv0)
{
	fprintf(fp, "Usage: %s [OPTION]... SOURCE\n", argv0);
	fprintf(fp, "Merge the files from SOURCE onto the current system.\n");
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
	setvbuf(stdout, NULL, _IOLBF, 0); // Pipes.

	bool cancel = false;
	bool booting = false;
	bool wait = false;

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
			case 'w': wait = true; break;
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
		else if ( !strcmp(arg, "--cancel") )
			cancel = true;
		else if ( !strcmp(arg, "--booting") )
			booting = true;
		else if ( !strcmp(arg, "--wait") )
			wait = true;
		else
		{
			fprintf(stderr, "%s: unknown option: %s\n", argv0, arg);
			help(stderr, argv0);
			exit(1);
		}
	}

	compact_arguments(&argc, &argv);

	const char* source;
	if ( cancel )
	{
		source = NULL;
		if ( 1 < argc )
			err(2, "Unexpected extra operand `%s'", argv[2]);
	}
	else if ( booting )
	{
		source = "/sysmerge";
		if ( 1 < argc )
			err(2, "Unexpected extra operand `%s'", argv[2]);
	}
	else
	{
		if ( argc < 2 )
			err(2, "No source operand was given");
		source = argv[1];
		if ( 2 < argc )
			err(2, "Unexpected extra operand `%s'", argv[2]);
	}

	if ( booting )
	{
	}
	else if ( has_pending_upgrade() )
	{
		rename("/boot/sortix.bin.sysmerge.orig", "/boot/sortix.bin");
		rename("/boot/sortix.initrd.sysmerge.orig", "/boot/sortix.initrd");
		execute((const char*[]) { "rm", "-rf", "/sysmerge", NULL }, "");
		execute((const char*[]) { "update-initrd", NULL }, "_e");
		printf("Cancelled pending system upgrade.\n");
	}
	else if ( cancel )
		printf("No system upgrade was pending.\n");

	if ( cancel )
		return 0;

	const char* old_release_path = "/etc/sortix-release";
	struct release old_release;
	if ( !os_release_load(&old_release, old_release_path, old_release_path) )
		exit(2);

	char* new_release_path;
	if ( asprintf(&new_release_path, "%s/etc/sortix-release", source) < 0 )
		err(2, "asprintf");
	struct release new_release;
	if ( !os_release_load(&new_release, new_release_path, new_release_path) )
		exit(2);
	free(new_release_path);

	// TODO: Check if /etc/machine matches the current architecture.
	// TODO: Check for version (skipping, downgrading).

	struct conf conf;
	load_upgrade_conf(&conf, "/etc/upgrade.conf");

	bool can_run_old_abi = old_release.abi_major == new_release.abi_major &&
	                       old_release.abi_minor <= new_release.abi_minor;
	if ( !can_run_old_abi && !wait )
	{
		printf("Incompatible %lu.%lu -> %lu.%lu ABI transition, "
		       "delaying upgrade to next boot.\n",
		       old_release.abi_major, old_release.abi_major,
		       new_release.abi_major, new_release.abi_major);
		wait = true;
	}

	const char* target;
	if ( wait )
	{
		printf("Scheduling upgrade to %s on next boot using %s:\n",
		       new_release.pretty_name, source);
		target = "/sysmerge";
		if ( mkdir(target, 0755) < 0 )
			err(2, "%s", target);
		execute((const char*[]) { "tix-collection", "/sysmerge", "create",
		                          NULL }, "_e");
	}
	else
	{
		printf("Upgrading to %s using %s:\n", new_release.pretty_name, source);
		target = "";
	}

	install_manifest("system", source, target);
	install_ports(source, target);

	if ( wait )
	{
		printf(" - Scheduling upgrade on next boot...\n");
		execute((const char*[]) { "cp", "/boot/sortix.bin",
		                                "/boot/sortix.bin.sysmerge.orig" }, "_e");
		execute((const char*[]) { "cp", "/boot/sortix.initrd",
		                                "/boot/sortix.initrd.sysmerge.orig" }, "_e");
		execute((const char*[]) { "cp", "/sysmerge/boot/sortix.bin",
		                                "/boot/sortix.bin" }, "_e");
		execute((const char*[]) { "/sysmerge/sbin/update-initrd", NULL }, "_e");

		printf("The system will be upgraded to %s on the next boot.\n",
		       new_release.pretty_name);
		printf("Run %s --cancel to cancel the upgrade.\n", argv[0]);

		return 0;
	}

	if ( !wait && access_or_die("/etc/fstab", F_OK) == 0 )
	{
		printf(" - Creating initrd...\n");
		execute((const char*[]) { "update-initrd", NULL }, "_e");

		if ( conf.grub )
		{
			// TODO: Figure out the root device.
			//printf(" - Installing bootloader...\n");
			//execute((const char*[]) { "grub-install", "/", NULL }, "_eqQ");
			printf(" - Configuring bootloader...\n");
			execute((const char*[]) { "update-grub", NULL }, "_eqQ");
		}
		else if ( access_or_die("/etc/grub.d/10_sortix", F_OK) == 0 )
		{
			printf(" - Creating bootloader fragment...\n");
			execute((const char*[]) { "/etc/grub.d/10_sortix", NULL }, "_eq");
		}
	}

	if ( booting )
	{
		unlink("/boot/sortix.bin.sysmerge.orig");
		unlink("/boot/sortix.initrd.sysmerge.orig");
		execute((const char*[]) { "rm", "-rf", "/sysmerge", NULL }, "");
	}

	printf("Successfully upgraded to %s.\n", new_release.pretty_name);

	return 0;
}
