/*
 * Copyright (c) 2013 Jonas 'Sortie' Termansen.
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
 * du.c
 * Estimate file space usage.
 */

// TODO: Currently hardlinks count twice!

#include <sys/stat.h>
#include <sys/types.h>

#include <dirent.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const int FLAG_ALL = 1 << 0;
static const int FLAG_SUMMARIZE = 1 << 1;
static const int FLAG_SAME_DEVICE = 1 << 2;
static const int FLAG_APPARENT_SIZE = 1 << 3;
static const int FLAG_TOTAL = 1 << 4;
static const int FLAG_HUMAN_READABLE = 1 << 5;
static const int FLAG_SI = 1 << 6;
static const int FLAG_SEPARATE_DIRS = 1 << 7;
static const int FLAG_IS_OPERAND = 1 << 8;

enum symbolic_dereference
{
	SYMBOLIC_DEREFERENCE_NONE,
	SYMBOLIC_DEREFERENCE_ARGUMENTS,
	SYMBOLIC_DEREFERENCE_ALWAYS,
};

static bool string_has_prefix(const char* string, const char* prefix)
{
	return !strncmp(string, prefix, strlen(prefix));
}

static char* append_to_path(const char* path, const char* elem)
{
	size_t path_length = strlen(path);
	if ( !path_length )
		return errno = EINVAL, (char*) NULL;
	size_t elem_length = strlen(elem);
	size_t result_length = path_length + elem_length;
	const char* extra_slash = "";
	if ( path[path_length-1] != '/' )
	{
		result_length += 1;
		extra_slash = "/";
	}
	char* result = (char*) malloc(sizeof(char) * (result_length+1));
	if ( !result )
		return NULL;
	stpcpy(stpcpy(stpcpy(result, path), extra_slash), elem);
	return result;
}

static uintmax_t size_to_blocks(off_t size, uintmax_t block_size)
{
	uintmax_t umax_size = (uintmax_t) size;
	return umax_size / block_size + (umax_size % block_size ? 1 : 0);
}

static uintmax_t parse_block_size(const char* block_size_str)
{
	if ( !block_size_str[0] )
		return 0;
	const char* endptr;
	uintmax_t ret = strtoumax((char*) block_size_str, (char**) &endptr, 0);
	if ( !ret && block_size_str[0] == '0' )
		return 0;
	if ( endptr[0] && endptr[1] && (endptr[1] != 'B' || endptr[2]) )
		return 0;
	uintmax_t magnitude = 1;
	uintmax_t exponent = endptr[0] && endptr[1] == 'B' ? 1000 : 1024;
	char prefixes[] = { '\0', 'K', 'M', 'G', 'T', 'P', 'E', 'Z', 'Y' };
	size_t num_prefixes = sizeof(prefixes) / sizeof(prefixes[0]);
	size_t prefix_index;
	for ( prefix_index = 0;
	      *endptr != prefixes[prefix_index] && prefix_index < num_prefixes;
	      prefix_index++ )
		magnitude *= exponent;
	if ( prefix_index == num_prefixes )
		return 0;
	if ( !ret )
		ret = 1;
	return ret * magnitude;
}

static void print_disk_usage(uintmax_t num_bytes, uintmax_t block_size,
                             int flags, const char* path)
{
	if ( flags & FLAG_HUMAN_READABLE )
	{
		uintmax_t value = num_bytes;
		uintmax_t exponent = flags & FLAG_SI ? 1000 : 1024;
		char prefixes[] = { '\0', 'K', 'M', 'G', 'T', 'P', 'E', 'Z', 'Y' };
		size_t num_prefixes = sizeof(prefixes) / sizeof(prefixes[0]);
		size_t prefix_index = 0;
		while ( exponent <= value && prefix_index + 1 < num_prefixes)
		{
			value /= exponent;
			prefix_index++;
		}
		char prefix_str[2] = { prefixes[prefix_index], '\0' };
		printf("%ju%s\t%s\n", value, prefix_str, path);
	}
	else
	{
		uintmax_t num_blocks = size_to_blocks(num_bytes, block_size);
		printf("%ju\t%s\n", num_blocks, path);
	}
}

static
bool disk_usage_file_at(int relfd,
                        const char* relpath,
                        const char* path,
                        int flags,
                        enum symbolic_dereference symbolic_dereference,
                        uintmax_t block_size,
                        uintmax_t* total_bytes_ptr,
                        uintmax_t* num_bytes_ptr,
                        dev_t expected_dev,
                        mode_t* result_mode_ptr)
{
	bool flag_all = flags & FLAG_ALL;
	bool flag_is_operand = flags & FLAG_IS_OPERAND;
	bool flag_same_device = flags & FLAG_SAME_DEVICE;
	bool flag_separate_dirs = flags & FLAG_SEPARATE_DIRS;
	bool flag_summarize = flags & FLAG_SUMMARIZE;

	bool print_if_file = flag_is_operand || (!flag_summarize && flag_all);
	bool print_if_dir = (flags & FLAG_IS_OPERAND) || !(flags & FLAG_SUMMARIZE);

	bool follow_symlinks =
		symbolic_dereference == SYMBOLIC_DEREFERENCE_ALWAYS ||
		(flag_is_operand && symbolic_dereference == SYMBOLIC_DEREFERENCE_ARGUMENTS);

	int open_flags = O_RDONLY | (!follow_symlinks ? O_NOFOLLOW : 0);
	int fd = openat(relfd, relpath, open_flags);
	if ( fd < 0 )
	{
		if ( errno == ELOOP && !follow_symlinks )
		{
			if ( print_if_file )
				print_disk_usage(0, block_size, flags, path);
			if ( num_bytes_ptr )
				*num_bytes_ptr = 0;
			if ( result_mode_ptr )
				*result_mode_ptr = S_IFLNK;
			return true;
		}
		return error(0, errno, "cannot access `%s'", path), false;
	}

	struct stat st;
	if ( fstat(fd, &st) != 0 )
	{
		error(0, errno, "stat: `%s'", path);
		close(fd);
		return false;
	}

	if ( result_mode_ptr )
		*result_mode_ptr = st.st_mode;

	if ( flag_is_operand )
		expected_dev = st.st_dev;
	else if ( flag_same_device && st.st_dev != expected_dev )
		return true;

	uintmax_t num_bytes = S_ISBLK(st.st_mode) ?
	                      0 :
	                      flags & FLAG_APPARENT_SIZE ?
	                      (intmax_t) st.st_size :
	                      st.st_blocks * 512;

	if ( !S_ISDIR(st.st_mode) )
	{
		if ( print_if_file )
			print_disk_usage(num_bytes, block_size, flags, path);
		if ( num_bytes_ptr )
			*num_bytes_ptr = num_bytes;
		if ( total_bytes_ptr )
			*total_bytes_ptr += num_bytes;
		close(fd);
		return true;
	}

	if ( total_bytes_ptr )
		*total_bytes_ptr += num_bytes;

	DIR* dir = fdopendir(fd);
	if ( !dir )
	{
		error(0, errno, "fdopendir(%i): `%s'", fd, path);
		close(fd);
		return false;
	}

	bool success = true;
	struct dirent* entry;
	while ( (errno = 0, entry = readdir(dir)) )
	{
		if ( !strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..") )
			continue;
		char* new_path = append_to_path(path, entry->d_name);
		if ( !new_path )
		{
			error(0, errno, "malloc: `%s/%s'", path, entry->d_name);
			continue;
		}
		int new_flags = flags & ~FLAG_IS_OPERAND;
		uintmax_t new_num_bytes = 0;
		mode_t new_mode = 0;
		if ( !disk_usage_file_at(dirfd(dir), entry->d_name, new_path, new_flags,
		                         symbolic_dereference, block_size,
		                         total_bytes_ptr, &new_num_bytes, expected_dev,
		                         &new_mode) )
			success = false;
		if ( !flag_separate_dirs || !S_ISDIR(new_mode) )
			num_bytes += new_num_bytes;
		free(new_path);
	}

	if ( num_bytes_ptr )
		*num_bytes_ptr = num_bytes;

	if ( errno && errno != ENOTDIR )
	{
		error(0, errno, "reading directory `%s'", path);
		closedir(dir);
		return false;
	}

	if ( print_if_dir )
		print_disk_usage(num_bytes, block_size, flags, path);

	closedir(dir);

	return success;
}

static void help(FILE* fp, const char* argv0)
{
	fprintf(fp, "Usage: %s [OPTION]... [FILE]...\n", argv0);
	fprintf(fp, "Summarize disk usage of each FILE, recursively for directories.\n");
	fprintf(fp, "\n");
	fprintf(fp, "Mandatory arguments to long options are mandatory for short options too.\n");
	fprintf(fp, "  -a, --all             write counts for all files, not just directories\n");
	fprintf(fp, "      --apparent-size   print apparent sizes, rather than disk usage; although\n");
	fprintf(fp, "                          the apparent size is usually smaller, it may be\n");
	fprintf(fp, "                          larger due to holes in (`sparse') files, internal\n");
	fprintf(fp, "                          fragmentation, indirect blocks, and the like\n");
	fprintf(fp, "  -B, --block-size=SIZE  scale sizes by SIZE before printing them.  E.g.,\n");
	fprintf(fp, "                           `-BM' prints sizes in units of 1,048,576 bytes.\n");
	fprintf(fp, "                           See SIZE format below.\n");
	fprintf(fp, "  -b, --bytes           equivalent to `--apparent-size --block-size=1'\n");
	fprintf(fp, "  -c, --total           produce a grand total\n");
	fprintf(fp, "  -D, --dereference-args  dereference only symlinks that are listed on the\n");
	fprintf(fp, "                          command line\n");
	fprintf(fp, "  -H                    equivalent to --dereference-args (-D)\n");
	fprintf(fp, "  -h, --human-readable  print sizes in human readable format (e.g., 1K 234M 2G)\n");
	fprintf(fp, "      --si              like -h, but use powers of 1000 not 1024\n");
	fprintf(fp, "  -k                    like --block-size=1K\n");
	fprintf(fp, "  -m                    like --block-size=1M\n");
	fprintf(fp, "  -L, --dereference     dereference all symbolic links\n");
	fprintf(fp, "  -P, --no-dereference  don't follow any symbolic links (this is the default)\n");
	fprintf(fp, "  -S, --separate-dirs   do not include size of subdirectories\n");
	fprintf(fp, "  -s, --summarize       display only a total for each argument\n");
	fprintf(fp, "  -x, --one-file-system skip directories on different file systems\n");
	fprintf(fp, "      --help            display this help and exit\n");
	fprintf(fp, "      --version         output version information and exit\n");
	fprintf(fp, "\n");
	fprintf(fp, "Display values are in units of the first available SIZE from --block-size,\n");
	fprintf(fp, "and the DU_BLOCK_SIZE, BLOCK_SIZE and BLOCKSIZE environment variables.\n");
	fprintf(fp, "Otherwise, units default to 1024 bytes (or 512 if POSIXLY_CORRECT is set).\n");
	fprintf(fp, "\n");
	fprintf(fp, "SIZE may be (or may be an integer optionally followed by) one of following:\n");
	fprintf(fp, "KB 1000, K 1024, MB 1000*1000, M 1024*1024, and so on for G, T, P, E, Z, Y.\n");
}

static void version(FILE* fp, const char* argv0)
{
	fprintf(fp, "%s (Sortix) %s\n", argv0, VERSIONSTR);
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

static
bool disk_usage_files(int argc,
                      char* argv[],
                      int flags,
                      enum symbolic_dereference symbolic_dereference,
                      uintmax_t block_size)
{
	bool success = true;

	uintmax_t total_bytes = 0;

	if ( argc <= 1 )
	{
		if ( !disk_usage_file_at(AT_FDCWD, ".", ".", flags | FLAG_IS_OPERAND,
		                         symbolic_dereference, block_size, &total_bytes,
		                         NULL, 0, NULL) )
			success = false;
	}
	else for ( int i = 1; i < argc; i++ )
	{
		const char* path = argv[i];
		if ( !disk_usage_file_at(AT_FDCWD, path, path, flags | FLAG_IS_OPERAND,
		                         symbolic_dereference, block_size, &total_bytes,
		                         NULL, 0, NULL) )
			success = false;
	}

	if ( flags & FLAG_TOTAL )
		print_disk_usage(total_bytes, block_size, flags, "total");

	return success;
}

static uintmax_t get_default_block_size(void)
{
	uintmax_t result = 0;
	if ( !result && getenv("DU_BLOCK_SIZE") )
		result = parse_block_size(getenv("DU_BLOCK_SIZE"));
	if ( !result && getenv("BLOCK_SIZE") )
		result = parse_block_size(getenv("BLOCK_SIZE"));
	if ( !result && getenv("BLOCKSIZE") )
		result = parse_block_size(getenv("BLOCKSIZE"));
	if ( !result && getenv("POSIXLY_CORRECT") )
		result = 512;
	if ( !result )
		result = 1024;
	return result;
}

int main(int argc, char* argv[])
{
	int flags = 0;
	enum symbolic_dereference symbolic_dereference = SYMBOLIC_DEREFERENCE_NONE;
	uintmax_t block_size = get_default_block_size();

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
			case 'a': flags |= FLAG_ALL; break;
			case 'b': flags |= FLAG_APPARENT_SIZE, block_size = 1; break;
			case 'B':
				if ( !arg[1] )
					goto next_is_block_size_operand;
				if ( !(block_size = parse_block_size(arg+1)) )
					error(1, 0, "invalid block size `%s'", arg);
				arg += strlen(arg)-1;
				break;
			case 'c': flags |= FLAG_TOTAL; break;
			case 'D': symbolic_dereference = SYMBOLIC_DEREFERENCE_ARGUMENTS; break;
			case 'h': flags |= FLAG_HUMAN_READABLE; break;
			case 'H': symbolic_dereference = SYMBOLIC_DEREFERENCE_ARGUMENTS; break;
			case 'k': block_size = 1024; break;
			case 'L': symbolic_dereference = SYMBOLIC_DEREFERENCE_ALWAYS; break;
			case 'm': block_size = 1024*1024; break;
			case 'P': symbolic_dereference = SYMBOLIC_DEREFERENCE_NONE; break;
			case 's': flags |= FLAG_SUMMARIZE; break;
			case 'S': flags |= FLAG_SEPARATE_DIRS; break;
			case 'x': flags |= FLAG_SAME_DEVICE; break;
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
		else if ( !strcmp(arg, "--all") )
			flags |= FLAG_ALL;
		else if ( !strcmp(arg, "--apparent-size") )
			flags |= FLAG_APPARENT_SIZE;
		else if ( !strcmp(arg, "--block-size") )
		{
		next_is_block_size_operand:
			if ( i + 1 == argc )
				error(1, 0, "expected operand after `%s'", arg);
			const char* block_size_str = argv[++i];
			argv[i] = NULL;
			if ( !(block_size = parse_block_size(block_size_str)) )
				error(1, 0, "invalid block size `%s'", block_size_str);
		}
		else if ( string_has_prefix(arg, "--block-size=") )
		{
			const char* block_size_str = arg + strlen("--block-size=");
			if ( !(block_size = parse_block_size(block_size_str)) )
				error(1, 0, "invalid block size `%s'", block_size_str);
		}
		else if ( !strcmp(arg, "--bytes") )
			flags |= FLAG_APPARENT_SIZE, block_size = 1;
		else if ( !strcmp(arg, "--dereference") )
			symbolic_dereference = SYMBOLIC_DEREFERENCE_ALWAYS;
		else if ( !strcmp(arg, "--dereference-args") )
			symbolic_dereference = SYMBOLIC_DEREFERENCE_ARGUMENTS;
		else if ( !strcmp(arg, "--human-readable") )
			flags |= FLAG_HUMAN_READABLE;
		else if ( !strcmp(arg, "--no-dereference") )
			symbolic_dereference = SYMBOLIC_DEREFERENCE_NONE;
		else if ( !strcmp(arg, "--one-file-system") )
			flags |= FLAG_SAME_DEVICE;
		else if ( !strcmp(arg, "--separate-dirs") )
			flags |= FLAG_SEPARATE_DIRS;
		else if ( !strcmp(arg, "--si") )
			flags |= FLAG_HUMAN_READABLE | FLAG_SI;
		else if ( !strcmp(arg, "--summarize") )
			flags |= FLAG_SUMMARIZE;
		else if ( !strcmp(arg, "--total") )
			flags |= FLAG_TOTAL;
		else
		{
			fprintf(stderr, "%s: unknown option: %s\n", argv0, arg);
			help(stderr, argv0);
			exit(1);
		}
	}

	compact_arguments(&argc, &argv);

	return disk_usage_files(argc, argv, flags, symbolic_dereference,
	                        block_size) ? 0 : 1;
}
