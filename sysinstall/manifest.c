/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2015, 2016.

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

    manifest.c
    Manifest handling functions.

*******************************************************************************/

#include <sys/types.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <ioleast.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "execute.h"
#include "fileops.h"
#include "manifest.h"

bool has_manifest(const char* manifest)
{
	char* path;
	if ( asprintf(&path, "/tix/manifest/%s", manifest) < 0 )
	{
		warn("asprintf");
		_exit(2);
	}
	bool result = access(path, F_OK) == 0;
	free(path);
	return result;
}

struct hardlink
{
	dev_t dev;
	ino_t ino;
	char* path;
};

void install_manifest(const char* manifest,
                      const char* from_prefix,
                      const char* to_prefix)
{
	printf(" - Installing %s...\n", manifest);
	struct hardlink* hardlinks = NULL;
	size_t hardlinks_used = 0;
	size_t hardlinks_length = 0;
	size_t buffer_size = 1 << 16;
	char* buffer = malloc(buffer_size);
	if ( !buffer )
	{
		warn("malloc");
		_exit(2);
	}
	mode_t old_umask = umask(0000);
	char* inmanifest;
	if ( asprintf(&inmanifest, "%s/tix/manifest/%s", from_prefix, manifest) < 0 )
	{
		warn("asprintf");
		_exit(2);
	}
	char* outmanifest;
	if ( asprintf(&outmanifest, "%s/tix/manifest/%s", to_prefix, manifest) < 0 )
	{
		warn("asprintf");
		_exit(2);
	}
	FILE* fpin = fopen(inmanifest, "r");
	if ( !fpin )
	{
		warn("%s", inmanifest);
		_exit(2);
	}
	FILE* fpout = fopen(outmanifest, "w");
	if ( !fpout )
	{
		warn("%s", outmanifest);
		_exit(2);
	}
	char* line = NULL;
	size_t line_size = 0;
	ssize_t line_length;
	while ( 0 <= (errno = 0, line_length = getline(&line, &line_size, fpin)) )
	{
		if ( line_length && line[line_length-1] == '\n' )
			line[--line_length] = '\0';
		if ( fprintf(fpout, "%s\n", line) < 0 )
		{
			warn("write: %s", outmanifest);
			_exit(2);
		}
		if ( line[0] != '/' )
			continue;
		char* in_path;
		if ( asprintf(&in_path, "%s%s", from_prefix, line) < 0 )
		{
			warn("asprintf");
			_exit(2);
		}
		char* out_path = line;
		if ( asprintf(&out_path, "%s%s", to_prefix, line) < 0 )
		{
			warn("asprintf");
			_exit(2);
		}
		struct stat inst;
		if ( lstat(in_path, &inst) < 0 )
		{
			warn("%s", in_path);
			_exit(2);
		}
		struct hardlink* hardlink = NULL;
		if ( S_ISREG(inst.st_mode) && 2 <= inst.st_nlink )
		{
			for ( size_t i = 0; i < hardlinks_used; i++ )
			{
				if ( hardlinks[i].dev != inst.st_dev ||
				     hardlinks[i].ino != inst.st_ino )
					continue;
				hardlink = &hardlinks[i];
				break;
			}
		}
		if ( hardlink )
		{
			unlink(out_path);
			if ( link(hardlink->path, out_path) < 0 )
			{
				warn("link: %s -> %s", hardlink->path, out_path);
				_exit(2);
			}
		}
		else if ( S_ISDIR(inst.st_mode) )
		{
			if ( mkdir(out_path, inst.st_mode & 07777) < 0 && errno != EEXIST )
			{
				warn("mkdir: %s", out_path);
				_exit(2);
			}
		}
		else if ( S_ISREG(inst.st_mode) )
		{

			int in_fd = open(in_path, O_RDONLY);
			if ( in_fd < 0 )
			{
				warn("%s", in_path);
				_exit(2);
			}
			unlink(out_path);
			int out_fd = open(out_path, O_WRONLY | O_CREAT | O_TRUNC,
			                  inst.st_mode & 07777);
			if ( out_fd < 0 )
			{
				warn("%s", out_path);
				_exit(2);
			}
			while ( true )
			{
				ssize_t amount = read(in_fd, buffer, buffer_size);
				if ( amount < 0 )
				{
					warn("read: %s", in_path);
					_exit(2);
				}
				if ( amount == 0 )
					break;
				if ( writeall(out_fd, buffer, (size_t) amount) < (size_t) amount )
				{
					warn("write: %s", out_path);
					_exit(2);
				}
			}
			close(out_fd);
			close(in_fd);
			if ( 2 <= inst.st_nlink )
			{
				if ( hardlinks_used == hardlinks_length )
				{
					// TODO: Multiplication overflow.
					size_t new_length = hardlinks_length ? 2 * hardlinks_length : 16;
					struct hardlink* new_hardlinks = (struct hardlink*)
						reallocarray(hardlinks, new_length, sizeof(struct hardlink));
					if ( !new_hardlinks )
					{
						warn("malloc");
						_exit(2);
					}
					hardlinks = new_hardlinks;
					hardlinks_length = new_length;
				}
				hardlinks[hardlinks_used].ino = inst.st_ino;
				hardlinks[hardlinks_used].dev = inst.st_dev;
				if ( !(hardlinks[hardlinks_used].path = strdup(out_path)) )
				{
					warn("strdup");
					_exit(2);
				}
				hardlinks_used++;
			}
		}
		else if ( S_ISLNK(inst.st_mode) )
		{
			ssize_t amount = readlink(in_path, buffer, buffer_size - 1);
			if ( amount < 0 )
			{
				warn("readlink: %s", in_path);
				_exit(2);
			}
			buffer[amount] = '\0';
			unlink(out_path);
			if ( symlink(buffer, out_path) < 0 && errno != EEXIST )
			{
				warn("symlink: %s", out_path);
				_exit(2);
			}
		}
		else
		{
			warnx("%s: Don't know how to copy this object", in_path);
			_exit(2);
		}
		free(in_path);
		free(out_path);
	}
	free(line);
	if ( errno )
	{
		warn("%s", inmanifest);
		_exit(2);
	}
	fclose(fpin);
	if ( fclose(fpout) == EOF )
	{
		warn("close: %s", outmanifest);
		_exit(2);
	}
	free(inmanifest);
	free(outmanifest);
	umask(old_umask);
	free(buffer);
	for ( size_t i = 0; i < hardlinks_used; i++ )
		free(hardlinks[i].path);
	free(hardlinks);
}

bool check_installed(const char* path, const char* package)
{
	FILE* fp = fopen(path, "r");
	if ( !fp )
	{
		if ( errno != ENOENT )
			warn("%s", path);
		return false;
	}
	char* line = NULL;
	size_t line_size = 0;
	ssize_t line_length;
	while ( 0 < (errno = 0, line_length = getline(&line, &line_size, fp)) )
	{
		if ( line[line_length-1] == '\n' )
			line[--line_length] = '\0';
		if ( !strcmp(line, package) )
		{
			free(line);
			fclose(fp);
			return true;
		}
	}
	if ( errno != 0 )
		warn("%s", path);
	free(line);
	fclose(fp);
	return false;
}

static char* shell_single_quote(const char* string)
{
	char* result;
	size_t result_size;
	FILE* fp = open_memstream(&result, &result_size);
	if (!fp)
		return NULL;
	fputc('\'', fp);
	for ( size_t i = 0; string[i]; i++ )
	{
		if ( string[i] == '\'' )
			fputs("\'\\\'\'", fp);
		else
			fputc((unsigned char) string[i], fp);
	}
	fputc('\'', fp);
	fflush(fp);
	int waserr = ferror(fp);
	fclose(fp);
	if (waserr) {
		free(result);
		return NULL;
	}
	return result;
}

static char* sort_file_cmd(const char* file)
{
	char* file_esc = shell_single_quote(file);
	if ( !file_esc )
		return NULL;
	char* cmd;
	if ( asprintf(&cmd, "sort -- %s", file_esc) < 0 )
	{
		free(file_esc);
		return NULL;
	}
	free(file_esc);
	return cmd;
}

void install_ports(const char* from_prefix, const char* to_prefix)
{
	char* inst_in_path;
	char* inst_out_path;
	if ( asprintf(&inst_in_path, "%s/tix/installed.list", from_prefix) < 0 ||
	     asprintf(&inst_out_path, "%s/tix/installed.list", to_prefix) < 0 )
	{
		warn("asprintf");
		_exit(2);
	}
	if ( access_or_die(inst_in_path, F_OK) < 0 )
	{
		free(inst_in_path);
		free(inst_out_path);
		return;
	}
	char* cmd = sort_file_cmd(inst_in_path);
	if ( !cmd )
	{
		warn("sort_file_cmd");
		_exit(2);
	}
	FILE* fp = popen(cmd, "r");
	if ( !fp )
	{
		warn("%s", cmd);
		_exit(2);
	}
	char* line = NULL;
	size_t line_size = 0;
	ssize_t line_length;
	while ( 0 < (errno = 0, line_length = getline(&line, &line_size, fp)) )
	{
		if ( line[line_length-1] == '\n' )
			line[--line_length] = '\0';
		if ( !check_installed(inst_out_path, line) )
		{
			FILE* inst_out_fp = fopen(inst_out_path, "a");
			if ( !inst_out_fp ||
				 fprintf(inst_out_fp, "%s\n", line) < 0 ||
				 fflush(inst_out_fp) == EOF )
			{
				warn("%s", inst_out_path);
				pclose(fp);
				_exit(2);
			}
			fclose(inst_out_fp);
		}
		char* tixinfo_in;
		char* tixinfo_out;
		if ( asprintf(&tixinfo_in, "%s/tix/tixinfo/%s", from_prefix, line) < 0 ||
		     asprintf(&tixinfo_out, "%s/tix/tixinfo/%s", to_prefix, line) < 0 )
		{
			warn("asprintf");
			pclose(fp);
			_exit(2);
		}
		execute((const char*[]) { "cp", "--", tixinfo_in, tixinfo_out, NULL }, "_e");
		free(tixinfo_in);
		free(tixinfo_out);
		install_manifest(line, from_prefix, to_prefix);
	}
	free(line);
	if ( errno )
	{
		warn("%s", cmd);
		pclose(fp);
		_exit(2);
	}
	pclose(fp);
	free(cmd);
	free(inst_in_path);
	free(inst_out_path);
}
