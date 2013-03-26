/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

    This file is part of Tix.

    Tix is free software: you can redistribute it and/or modify it under the
    terms of the GNU General Public License as published by the Free Software
    Foundation, either version 3 of the License, or (at your option) any later
    version.

    Tix is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
    details.

    You should have received a copy of the GNU General Public License along with
    Tix. If not, see <https://www.gnu.org/licenses/>.

    tix-object-insert.cpp
    Inserts files into a tix object database.

*******************************************************************************/

#include <sys/stat.h>
#include <sys/types.h>

#include <assert.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <nettle/sha.h>

int create_and_open_directory_at(int dirfd, const char* path, mode_t mode)
{

	int ret = openat(dirfd, path, O_RDONLY | O_DIRECTORY);
	if ( ret < 0 && errno == EEXIST )
	{
		if ( mkdirat(dirfd, path, mode) != 0 )
			return -1;
		return openat(dirfd, path, O_RDONLY | O_DIRECTORY);
	}
	return ret;
}

struct options
{
	bool hash;
	bool quiet;
	bool link;
	bool symlink;
};

bool insert_object(struct options* opts,
                   const char* file_path,
                   const char* tmp,
                   int database_fd, const char* database_path,
                   const char* digest,
                   int dir_fd, const char* dir_name)
{
	const char* entry_name = digest + 2;
	if ( linkat(database_fd, tmp, dir_fd, entry_name, 0) &&
	     errno != EEXIST )
	{
		error(0, errno, "`%s/%s' -> `%s/%s/%s'", database_path, dir_name,
		      database_path, dir_name, entry_name);
		return false;
	}

	if ( opts->hash )
		printf("%s\n", digest);
	else if ( !opts->quiet )
		printf("`%s' -> `%s/%s/%s'\n", file_path,
		                               database_path, dir_name, entry_name);

	return true;
}

bool insert_object(struct options* opts,
                   const char* file_path,
                   const char* tmp,
                   int database_fd, const char* database_path,
                   const char* digest)
{
	char dir_name[3] = { digest[0], digest[1], '\0' };
	int dir_fd = create_and_open_directory_at(database_fd, dir_name, 0777);
	if ( dir_fd < 0 )
	{
		error(0, errno, "`%s/%s'", database_path, dir_name);
		return false;
	}

	bool success = insert_object(opts, file_path, tmp, database_fd, database_path, digest, dir_fd, dir_name);

	close(dir_fd);

	return success;
}

bool copy_and_hash(FILE* fpin, const char* file_path,
                   FILE* fpout, const char* tmp,
                   int /*database_fd*/, const char* database_path,
                   char* digest)
{
	struct sha256_ctx shactx;
	sha256_init(&shactx);

	uint8_t buffer[SHA256_DATA_SIZE];
	while ( size_t num_bytes = fread(buffer, 1, sizeof(buffer), fpin) )
	{
		assert(num_bytes <= UINT_MAX);
		sha256_update(&shactx, num_bytes, buffer);
		if ( fwrite(buffer, 1, num_bytes, fpout) != num_bytes )
		{
			error(0, errno, "`%s/%s'", database_path, tmp);
			return false;
		}
	}

	if ( ferror(fpin) )
	{
		error(0, errno, "`%s'", file_path);
		return false;
	}

	if ( fflush(fpout) != 0 )
	{
		error(0, errno, "`%s/%s'", database_path, tmp);
		return false;
	}

	uint8_t binary_digest[SHA256_DIGEST_SIZE];
	sha256_digest(&shactx, sizeof(binary_digest), binary_digest);

	for ( size_t n = 0; n < SHA256_DIGEST_SIZE; n++ )
		snprintf(digest + 2 * n, 3, "%02x", binary_digest[n]);

	return true;
}

bool insert_object(struct options* opts,
                   FILE* fpin, const char* file_path,
                   FILE* fpout, const char* tmp,
                   int database_fd, const char* database_path)
{
	char digest[2 * SHA256_DIGEST_SIZE + 1];

	if ( !copy_and_hash(fpin, file_path, fpout, tmp, database_fd,
	                    database_path, digest) )
		return false;

	return insert_object(opts, file_path, tmp, database_fd, database_path, digest);
}

// TODO: Preferably use O_TMPFILE to avoid naming the file until we have made a
//       copy whose hash we know.
bool insert_object(struct options* opts,
                   FILE* fpin, const char* file_path,
                   int database_fd, const char* database_path)
{
	char tmp[8 + 1 + sizeof(pid_t) * 3 + 1];
	snprintf(tmp, sizeof(tmp), "incoming.%ji", (intmax_t) getpid());
	int tmp_fd = openat(database_fd, tmp, O_CREAT | O_WRONLY | O_EXCL, 0444);
	if ( tmp_fd < 0 )
	{
		error(0, errno, "`%s/%s'", database_path, tmp);
		return false;
	}

	FILE* fpout = fdopen(tmp_fd, "w");
	if ( !fpout )
	{
		error(0, errno, "fdopen(%i)", tmp_fd);
		close(tmp_fd);
		return false;
	}

	bool success = insert_object(opts, fpin, file_path, fpout, tmp, database_fd, database_path);

	fclose(fpout);

	unlinkat(database_fd, tmp, 0);

	return success;
}

bool insert_object(struct options* opts,
                   const char* file_path,
                   int database_fd, const char* database_path)
{
	if ( !strcmp(file_path, "-") )
		return insert_object(opts, stdin, file_path, database_fd, database_path);

	FILE* fpin = fopen(file_path, "r");
	if ( !fpin )
		error(1, errno, "`%s'", file_path);

	bool success = insert_object(opts, fpin, file_path, database_fd, database_path);

	fclose(fpin);

	return success;
}

void help(FILE* fp, const char* argv0)
{
	fprintf(fp, "Usage: %s [OPTION]... [-l | -s] --database DATABASE FILE...\n", argv0);
	fprintf(fp, "Inserts files into a tix object database.\n");
}

void usage(FILE* fp, const char* argv0)
{
	help(fp, argv0);
}

void version(FILE* fp, const char* argv0)
{
	help(fp, argv0);
}

int main(int argc, char* argv[])
{
	const char* argv0 = argv[0];
	const char* database_path = NULL;
	bool hash = false;
	bool quiet = false;
	bool do_link = false;
	bool do_symlink = false;

	for ( int i = 0; i < argc; i++ )
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
			case 'h': hash = true;
			case 'l': do_link = true;
			case 'q': quiet = true;
			case 's': do_link = true;
			default:
				fprintf(stderr, "%s: unknown option -- '%c'\n", argv0, c);
				usage(stderr, argv0);
				exit(1);
			}
		}
		else if ( !strcmp(arg, "--help") )
			help(stdout, argv0), exit(0);
		else if ( !strcmp(arg, "--usage") )
			usage(stdout, argv0), exit(0);
		else if ( !strcmp(arg, "--version") )
			version(stdout, argv0), exit(0);
		else if ( !strcmp(arg, "--database") )
		{
			if ( i + 1 == argc )
				error(1, 0, "`--database' expected argument");
			database_path = argv[++i], argv[i] = NULL;
		}
		else if ( !strcmp(arg, "--hash") )
			hash = true;
		else if ( !strcmp(arg, "--link") )
			do_link = true;
		else if ( !strcmp(arg, "--quiet") )
			quiet = true;
		else if ( !strcmp(arg, "--symlink") )
			do_symlink = true;
		else
		{
			fprintf(stderr, "%s: unknown option: `%s'\n", argv0, arg);
			usage(stderr, argv0);
			exit(1);
		}
	}

	if ( argc == 1 )
		usage(stdout, argv0), exit(0);

	if ( do_link && do_symlink )
	{
		error(0, 0, "error: the -l and -s options are mutually exclusive");
		usage(stdout, argv0);
		exit(1);
	}

	if ( !database_path )
	{
		error(1, 0, "no `--database' option given, don't know what database");
		usage(stdout, argv0);
		exit(1);
	}

	int database_fd = open(database_path, O_RDONLY | O_DIRECTORY);
	if ( database_fd < 0 )
		error(1, errno, "`%s'", database_path);

	for ( int i = 1; i < argc; i++ )
	{
		const char* file_path = argv[i];
		if ( !file_path )
			continue;

		FILE* fpin = fopen(file_path, "r");
		if ( !fpin )
			error(1, errno, "`%s'", file_path);

		char tmp[8 + 1 + sizeof(pid_t) * 3 + 1];
		snprintf(tmp, sizeof(tmp), "incoming.%ju", (uintmax_t) getpid());
		int tmp_fd = openat(database_fd, tmp, O_CREAT | O_WRONLY | O_EXCL, 0444);
		if ( tmp_fd < 0 )
			error(1, errno, "`%s/%s'", database_path, tmp);
		FILE* fpout = fdopen(tmp_fd, "w");

		struct sha256_ctx shactx;
		sha256_init(&shactx);

		uint8_t buffer[SHA256_DATA_SIZE];
		while ( size_t num_bytes = fread(buffer, 1, sizeof(buffer), fpin) )
		{
			assert(num_bytes <= UINT_MAX);
			sha256_update(&shactx, num_bytes, buffer);
			if ( fwrite(buffer, 1, num_bytes, fpout) != num_bytes )
			{
				unlinkat(database_fd, tmp, 0);
				error(1, errno, "`%s/%s'", database_path, tmp);
			}
		}

		if ( ferror(fpin) )
		{
			unlinkat(database_fd, tmp, 0);
			error(1, errno, "`%s'", file_path);
		}
		fclose(fpin);

		if ( fflush(fpout) != 0 )
		{
			unlinkat(database_fd, tmp, 0);
			error(1, errno, "`%s/%s'", database_path, tmp);
		}

		fclose(fpout);

		uint8_t binary_digest[SHA256_DIGEST_SIZE];
		sha256_digest(&shactx, sizeof(binary_digest), binary_digest);
		char digest[2 * SHA256_DIGEST_SIZE + 1];
		for ( size_t n = 0; n < SHA256_DIGEST_SIZE; n++ )
			snprintf(digest + 2 * n, 3, "%02x", binary_digest[n]);

		char dir_name[3] = { digest[0], digest[1], '\0' };
		if ( mkdirat(database_fd, dir_name, 0777) != 0 &&
		     errno != EEXIST )
		{
			unlinkat(database_fd, tmp, 0);
			error(1, errno, "`%s/%s'", database_path, dir_name);
		}

		int dir_fd = openat(database_fd, dir_name, O_RDONLY | O_DIRECTORY);
		if ( dir_fd < 0 )
		{
			unlinkat(database_fd, tmp, 0);
			error(1, errno, "`%s/%s'", database_path, dir_name);
		}

		const char* entry_name = digest + 2;
		if ( linkat(database_fd, tmp, dir_fd, entry_name, 0) &&
		     errno != EEXIST )
		{
			unlinkat(database_fd, tmp, 0);
			error(1, errno, "`%s/%s' -> `%s/%s/%s'", database_path, dir_name,
			      database_path, dir_name, entry_name);
		}

		close(dir_fd);

		unlinkat(database_fd, tmp, 0);

		if ( hash )
			printf("%s\n", digest);
		else if ( !quiet )
			printf("`%s' -> `%s/%s/%s'\n", file_path,
			                               database_path, dir_name, entry_name);

		if ( do_link || do_symlink )
		{
			if ( unlink(file_path) < 0 )
				error(1, errno, "cannot unlink: `%s'", file_path);

			size_t link_dest_length = strlen(database_path) + 1 + strlen(dir_name) + strlen(entry_name);
			char* link_dest = (char*) malloc(sizeof(char) * (link_dest_length + 1));

			stpcpy(stpcpy(stpcpy(stpcpy(stpcpy(link_dest, database_path), "/"), dir_name), "/"), entry_name);

			if ( do_symlink )
			{
				if ( symlink(link_dest, file_path) < 0 )
					error(1, errno, "cannot symlink `%s' to `%s'", file_path,
					                link_dest);
			}
			else
			{
				if ( link(link_dest, file_path) < 0 )
					error(1, errno, "cannot link `%s' to `%s'", file_path,
					                link_dest);
			}

			free(link_dest);
		}
	}

	close(database_fd);

	return 0;
}
