/*
 * Copyright (c) 2015, 2016 Jonas 'Sortie' Termansen.
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
 * df.c
 * Report free disk space.
 */

#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>

#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <fstab.h>
#include <libgen.h>
#include <locale.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <wchar.h>

__attribute__((format(printf, 1, 2)))
static char* print_string(const char* format, ...)
{
	char* ret = NULL;
	va_list ap;
	va_start(ap, format);
	if ( vasprintf(&ret, format, ap) < 0 )
		ret = NULL;
	va_end(ap);
	return ret;
}

static char* adirname(const char* path)
{
	char* clone = strdup(path);
	if ( !clone )
		return NULL;
	char* dirnamed = dirname(clone);
	char* result = strdup(dirnamed);
	free(clone);
	return result;
}

static char* atcgetblob(int fd, const char* name, size_t* size_ptr)
{
	ssize_t size = tcgetblob(fd, name, NULL, 0);
	if ( size < 0 )
		return NULL;
	char* result = (char*) malloc((size_t) size + 1);
	if ( !result )
		return NULL;
	ssize_t second_size = tcgetblob(fd, name, result, (size_t) size);
	if ( second_size != size )
		return free(result), (char*) NULL;
	result[(size_t) size] = '\0';
	if ( size_ptr )
		*size_ptr = (size_t) size;
	return result;
}

static size_t string_display_length(const char* str)
{
	size_t display_length = 0;
	mbstate_t ps;
	memset(&ps, 0, sizeof(ps));
	while ( true )
	{
		wchar_t wc;
		size_t amount = mbrtowc(&wc, str, SIZE_MAX, &ps);
		if ( amount == 0 )
			break;
		if ( amount == (size_t) -1 || amount == (size_t) -2 )
		{
			display_length++;
			str++;
			memset(&ps, 0, sizeof(ps));
			continue;
		}
		int width = wcwidth(wc);
		if ( width < 0 )
			width = 0;
		if ( SIZE_MAX - display_length < (size_t) width )
			display_length = SIZE_MAX;
		else
			display_length += (size_t) width;
		str += amount;
	}
	return display_length;
}

static int open_canonical_directory(const char* path, char** out_path)
{
	char* canon = canonicalize_file_name(path);
	if ( !canon )
		return -1;
	int fd = open(path, O_RDONLY | O_DIRECTORY);
	if ( fd < 0 && errno == ENOTDIR )
	{
		char* parent_path = adirname(path);
		if ( parent_path )
		{
			if ( 0 <= (fd = open(parent_path, O_RDONLY | O_DIRECTORY)) )
			{
				free(canon);
				canon = parent_path;
				parent_path = NULL;
			}
			free(parent_path);
		}
	}
	if ( fd < 0 )
	{
		free(canon);
		return -1;
	}
	return *out_path = canon, fd;
}

static int open_mount_directory(int fd, const char* path, char** out_path)
{
	char* result_path = strdup(path);
	if ( !result_path )
		return -1;
	int result_fd = openat(fd, ".", O_RDONLY | O_DIRECTORY);
	if ( result_fd < 0 )
		return free(result_path), -1;
	struct stat result_st;
	if ( fstat(fd, &result_st) < 0 )
		return close(result_fd), free(result_path), -1;
	while ( true )
	{
		char* parent_path = adirname(result_path);
		if ( !parent_path )
			return close(result_fd), free(result_path), -1;
		if ( !strcmp(parent_path, result_path) )
		{
			free(parent_path);
			return *out_path = result_path, result_fd;
		}
		int parent_fd = openat(result_fd, "..", O_RDONLY | O_DIRECTORY);
		if ( parent_fd < 0 )
			return free(parent_path), close(result_fd), free(result_path), -1;
		struct stat parent_st;
		if ( fstat(parent_fd, &parent_st) < 0 )
			return close(parent_fd), free(parent_path), close(result_fd),
			       free(result_path), -1;
		if ( parent_st.st_dev != result_st.st_dev )
		{
			close(parent_fd);
			free(parent_path);
			return *out_path = result_path, result_fd;
		}
		close(result_fd);
		result_fd = parent_fd;
		free(result_path);
		result_path = parent_path;
		result_st = parent_st;
	}
}

struct df
{
	char* filesystem;
	char* type;
	char* mount;
	off_t blockdata_total;
	off_t blockdata_free;
	off_t blockdata_used;
	fsfilcnt_t inodes_total;
	fsfilcnt_t inodes_free;
	fsfilcnt_t inodes_used;
};

static void dftotal(struct df* total, const struct df* dfs, size_t dfs_count)
{
	memset(total, 0, sizeof(*total));
	total->filesystem = strdup("total");
	total->type = strdup("-");
	total->mount = strdup("-");
	for ( size_t i = 0; i < dfs_count; i++ )
	{
		total->blockdata_total += dfs[i].blockdata_total;
		total->blockdata_free += dfs[i].blockdata_free;
		total->blockdata_used += dfs[i].blockdata_used;
		total->inodes_total += dfs[i].inodes_total;
		total->inodes_free += dfs[i].inodes_free;
		total->inodes_used += dfs[i].inodes_used;
	}
}

static bool df(const char* path, struct df* buf)
{
	memset(buf, 0, sizeof(*buf));
	char* canon_fd_path;
	int canon_fd = open_canonical_directory(path, &canon_fd_path);
	if ( canon_fd < 0 )
	{
		error(0, errno, "%s", path);
		return false;
	}
	char* mount_fd_path;
	int mount_fd =
		open_mount_directory(canon_fd, canon_fd_path, &mount_fd_path);
	if ( mount_fd < 0 )
		error(0, errno, "finding mountpoint: %s", canon_fd_path);
	free(canon_fd_path);
	close(canon_fd);
	if ( mount_fd < 0 )
		return false;
	struct statvfs stvfs;
	if ( fstatvfs(mount_fd, &stvfs) < 0 )
	{
		error(0, errno, "statvfs: %s", mount_fd_path);
		return free(mount_fd_path), close(mount_fd), false;
	}
	if ( !(buf->filesystem = atcgetblob(mount_fd, "device-path", NULL)))
		buf->filesystem = strdup("none");
	if ( !(buf->type = atcgetblob(mount_fd, "filesystem-type", NULL)) )
		buf->filesystem = strdup("unknown");
	buf->mount = mount_fd_path;
	buf->blockdata_total = stvfs.f_bsize * stvfs.f_blocks;
	buf->blockdata_free = stvfs.f_bsize * stvfs.f_bfree;
	buf->blockdata_used = buf->blockdata_total - buf->blockdata_free;
	buf->inodes_total = stvfs.f_files;
	buf->inodes_free = stvfs.f_ffree;
	buf->inodes_used = buf->inodes_total - buf->inodes_free;
	close(mount_fd);
	return true;
}

static void dffree(struct df* buf)
{
	free(buf->filesystem);
	free(buf->type);
	free(buf->mount);
}

enum magnitude_format
{
	MAGFORMAT_DEFAULT,
	MAGFORMAT_512B,
	MAGFORMAT_1024B,
	MAGFORMAT_HUMAN_READABLE,
	MAGFORMAT_SI,
};

static char* format_bytes_amount(uintmax_t num_bytes, int base)
{
	uintmax_t value = num_bytes;
	uintmax_t value_fraction = 0;
	uintmax_t exponent = base;
	char prefixes[] = { '\0', 'K', 'M', 'G', 'T', 'P', 'E', 'Z', 'Y' };
	if ( base == 1000 )
		prefixes[1] = 'k';
	size_t num_prefixes = sizeof(prefixes) / sizeof(prefixes[0]);
	size_t prefix_index = 0;
	while ( exponent <= value && prefix_index + 1 < num_prefixes)
	{
		value_fraction = value % exponent;
		value /= exponent;
		prefix_index++;
	}
	char prefix_str[] = { prefixes[prefix_index], '\0' };
	int fix = base == 1024 ? 1 : 0;
	char value_fraction_char = '0' + (value_fraction / (base / 10 + fix)) % 10;
	char* result;
	if ( prefix_index == 0 &&
	     asprintf(&result, "%ju%s", value, prefix_str) < 0 )
		return NULL;
	if ( prefix_index != 0 &&
	     asprintf(&result, "%ju.%c%s", value, value_fraction_char,
	              prefix_str) < 0 )
		return NULL;
	return result;
}

static char* format_block_magnitude(off_t value,
                                    enum magnitude_format magformat)
{
	switch ( magformat )
	{
	case MAGFORMAT_DEFAULT: __builtin_unreachable();
	case MAGFORMAT_512B: return print_string("%ji", (intmax_t) (value / 512));
	case MAGFORMAT_1024B: return print_string("%ji", (intmax_t) (value / 1024));
	case MAGFORMAT_HUMAN_READABLE: return format_bytes_amount(value, 1024);
	case MAGFORMAT_SI: return format_bytes_amount(value, 1000);
	}
	__builtin_unreachable();
}

static char* format_inodes_magnitude(fsfilcnt_t value,
                                     enum magnitude_format magformat)
{
	switch ( magformat )
	{
	case MAGFORMAT_DEFAULT: __builtin_unreachable();
	case MAGFORMAT_512B: return print_string("%ju", (uintmax_t) value);
	case MAGFORMAT_1024B: return print_string("%ju", (uintmax_t) value);
	case MAGFORMAT_HUMAN_READABLE: return format_bytes_amount(value, 1024);
	case MAGFORMAT_SI: return format_bytes_amount(value, 1000);
	}
	__builtin_unreachable();
}

struct display_format
{
	enum magnitude_format magformat;
	bool inodes;
	bool portability;
	bool print_type;
};

enum column
{
	COLUMN_FILESYSTEM,
	COLUMN_TYPE,
	COLUMN_BLOCKS_TOTAL,
	COLUMN_BLOCKS_USED,
	COLUMN_BLOCKS_AVAILABLE,
	COLUMN_BLOCKS_USE_PERCENT,
	COLUMN_INODES_TOTAL,
	COLUMN_INODES_USED,
	COLUMN_INODES_AVAILABLE,
	COLUMN_INODES_USE_PERCENT,
	COLUMN_MOUNT,
	COLUMN_MAX_COLUMN,
};

static const char* label_column(enum column column,
                                struct display_format* format)
{
	switch ( column )
	{
	case COLUMN_FILESYSTEM: return "Filesystem";
	case COLUMN_TYPE: return "Type";
	case COLUMN_BLOCKS_TOTAL:
		switch ( format->magformat )
		{
		case MAGFORMAT_512B:
			return format->portability ? "512-blocks" : "512B-blocks";
		case MAGFORMAT_1024B:
			return format->portability ? "1024-blocks" : "1K-blocks";
		default:
			return "Size";
		}
	case COLUMN_BLOCKS_USED: return "Used";
	case COLUMN_BLOCKS_AVAILABLE:
		return format->magformat == MAGFORMAT_HUMAN_READABLE ||
		       format->magformat == MAGFORMAT_SI ? "Avail" : "Available";
	case COLUMN_BLOCKS_USE_PERCENT:
		return format->portability ? "Capacity" : "Use%";
	case COLUMN_INODES_TOTAL: return "Inodes";
	case COLUMN_INODES_USED: return "IUsed";
	case COLUMN_INODES_AVAILABLE: return "IFree";
	case COLUMN_INODES_USE_PERCENT: return "IUse%";
	case COLUMN_MOUNT: return "Mounted on";
	case COLUMN_MAX_COLUMN: __builtin_unreachable();
	}
	__builtin_unreachable();
}

static char* format_percent(uintmax_t a, uintmax_t b)
{
	if ( !b )
		return print_string("-");
	int percent = 100 * ((double) a / (double) b);
	if ( 100 < percent )
		percent = 100;
	return print_string("%i%%", percent);
}

static char* column_entry(struct df* buf,
                          enum column column,
                          struct display_format* format)
{
	switch ( column )
	{
	case COLUMN_FILESYSTEM:
		return strdup(buf->filesystem ? buf->filesystem : "none");
	case COLUMN_TYPE:
		return strdup(buf->type ? buf->type : "unknown");
	case COLUMN_BLOCKS_TOTAL:
		return format_block_magnitude(buf->blockdata_total, format->magformat);
	case COLUMN_BLOCKS_USED:
		return format_block_magnitude(buf->blockdata_used, format->magformat);
	case COLUMN_BLOCKS_AVAILABLE:
		return format_block_magnitude(buf->blockdata_free, format->magformat);
	case COLUMN_BLOCKS_USE_PERCENT:
		return format_percent(buf->blockdata_used, buf->blockdata_total);
	case COLUMN_INODES_TOTAL:
		return format_inodes_magnitude(buf->inodes_total, format->magformat);
	case COLUMN_INODES_USED:
		return format_inodes_magnitude(buf->inodes_used, format->magformat);
	case COLUMN_INODES_AVAILABLE:
		return format_inodes_magnitude(buf->inodes_free, format->magformat);
	case COLUMN_INODES_USE_PERCENT:
		return format_percent(buf->inodes_used, buf->inodes_total);
	case COLUMN_MOUNT:
		return strdup(buf->mount ? buf->mount : "unknown");
	case COLUMN_MAX_COLUMN: __builtin_unreachable();
	}
	__builtin_unreachable();
}

static void display_string(const char* string, size_t* position)
{
	fputs(string, stdout);
	*position += string_display_length(string);
}

static void display_indent(size_t indention, size_t* position)
{
	while ( *position < indention )
	{
		putchar(' ');
		(*position)++;
	}
}

static void display(struct df* dfs,
                    size_t dfs_count,
                    struct display_format* format)
{
	size_t good_ones = 0;
	for ( size_t i = 0; i < dfs_count; i++ )
		if ( dfs[i].mount )
			good_ones++;
	if ( good_ones == 0 )
		return;
	bool enable_column[COLUMN_MAX_COLUMN];
	enable_column[COLUMN_FILESYSTEM] = true;
	enable_column[COLUMN_TYPE] = format->print_type;
	enable_column[COLUMN_BLOCKS_TOTAL] = !format->inodes;
	enable_column[COLUMN_BLOCKS_USED] = !format->inodes;
	enable_column[COLUMN_BLOCKS_AVAILABLE] = !format->inodes;
	enable_column[COLUMN_BLOCKS_USE_PERCENT] = !format->inodes;
	enable_column[COLUMN_INODES_TOTAL] = format->inodes;
	enable_column[COLUMN_INODES_USED] = format->inodes;
	enable_column[COLUMN_INODES_AVAILABLE] = format->inodes;
	enable_column[COLUMN_INODES_USE_PERCENT] = format->inodes;
	enable_column[COLUMN_MOUNT] = true;
	size_t column_width[COLUMN_MAX_COLUMN];
	for ( int c = 0; c < COLUMN_MAX_COLUMN; c++ )
	{
		enum column column = (enum column) c;
		if ( !enable_column[column] )
			continue;
		size_t width = string_display_length(label_column(column, format));
		column_width[column] = width;
	}
	for ( size_t i = 0; i < dfs_count; i++ )
	{
		if ( !dfs[i].mount )
			continue;
		for ( int c = 0; c < COLUMN_MAX_COLUMN; c++ )
		{
			enum column column = (enum column) c;
			if ( !enable_column[column] )
				continue;
			char* entry = column_entry(&dfs[i], column, format);
			if ( !entry )
				error(1, errno, "malloc");
			size_t width = string_display_length(entry);
			free(entry);
			if ( column_width[column] < width )
				column_width[column] = width;
		}
	}
	size_t position = 0;
	size_t indention = 0;
	for ( int c = 0; c < COLUMN_MAX_COLUMN; c++ )
	{
		enum column column = (enum column) c;
		if ( !enable_column[column] )
			continue;
		display_indent(indention, &position);
		display_string(label_column(column, format), &position);
		indention += column_width[column] + 2;
	}
	printf("\n");
	for ( size_t i = 0; i < dfs_count; i++ )
	{
		if ( !dfs[i].mount )
			continue;
		position = 0;
		indention = 0;
		for ( int c = 0; c < COLUMN_MAX_COLUMN; c++ )
		{
			enum column column = (enum column) c;
			if ( !enable_column[column] )
				continue;
			char* entry = column_entry(&dfs[i], column, format);
			if ( !entry )
				error(1, errno, "malloc");
			display_indent(indention, &position);
			display_string(entry, &position);
			indention += column_width[column] + 2;
			free(entry);
		}
		printf("\n");
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

static void add_mount(char*** mounts_ptr,
                      size_t* mounts_used_ptr,
                      size_t* mounts_length_ptr,
                      const char* path)
{
	if ( *mounts_used_ptr == *mounts_length_ptr )
	{
		size_t new_length = 2 * *mounts_length_ptr;
		if ( new_length == 0 )
			new_length = 8;
		char** new_mounts =
			(char**) reallocarray(*mounts_ptr, new_length, sizeof(char**));
		if ( !new_mounts )
			error(1, errno, "malloc");
		*mounts_ptr = new_mounts;
		*mounts_length_ptr = new_length;
	}
	char* copy = strdup(path);
	if ( !copy )
		error(1, errno, "malloc");
	(*mounts_ptr)[(*mounts_used_ptr)++] = copy;
}

static bool is_mount(const char* path)
{
	int fd = open(path, O_RDONLY | O_DIRECTORY | O_NOFOLLOW);
	if ( fd < 0 )
	{
		if ( errno == ENOENT )
			return false;
		error(0, errno, "%s", path);
		return false;
	}
	struct stat fdst;
	if ( fstat(fd, &fdst) < 0 )
	{
		error(0, errno, "%s", path);
		close(fd);
		return false;
	}
	struct stat dotdotst;
	if ( fstatat(fd, "..", &dotdotst, 0) < 0 )
	{
		error(0, errno, "%s/..", path);
		close(fd);
		return false;
	}
	close(fd);
	if ( fdst.st_dev == dotdotst.st_dev )
		return fdst.st_ino == dotdotst.st_ino; /* root directory */
	return true;
}

static void get_mounts(char*** mounts, size_t* mounts_used)
{
	*mounts = NULL;
	*mounts_used = 0;
	size_t mounts_length = 0;
	if ( !setfsent() )
	{
		if ( errno != ENOENT )
			error(1, errno, "setfstab");
		add_mount(mounts, mounts_used, &mounts_length, "/");
		return;
	}
	struct fstab* fs;
	while ( (errno = 0, fs = getfsent()) )
	{
		if ( is_mount(fs->fs_file) )
			add_mount(mounts, mounts_used, &mounts_length, fs->fs_file);
	}
	if ( errno )
		error(1, errno, "getfsent");
	endfsent();
}

static int strcmp_indirect(const void* a_ptr, const void* b_ptr)
{
	const char* a = *(const char* const*) a_ptr;
	const char* b = *(const char* const*) b_ptr;
	return strcmp(a, b);
}

static void help(FILE* fp, const char* argv0)
{
	fprintf(fp, "Usage: %s [OPTION]... [FILE]...\n", argv0);
	fprintf(fp, "Write sorted concatenation of all FILE(s) to standard output.\n");
	fprintf(fp, "\n");
	fprintf(fp, "Mandatory arguments to long options are mandatory for short options too.\n");
	fprintf(fp, "  -a, --all             include dummy file systems\n");
#if 0
	fprintf(fp, "  -B, --block-size=SIZE  scale sizes by SIZE before printing them.  E.g.,\n");
	fprintf(fp, "                           '-BM' prints sizes in units of 1,048,576 bytes.\n");
	fprintf(fp, "                           See SIZE format below.\n");
#endif
	fprintf(fp, "      --total           produce a grand total\n");
	fprintf(fp, "  -h, --human-readable  print sizes in human readable format (e.g., 1K 234M 2G)\n");
	fprintf(fp, "  -H, --si              likewise, but use powers of 1000 not 1024\n");
	fprintf(fp, "  -i, --inodes          list inode information instead of block usage\n");
	fprintf(fp, "  -k                    like --block-size=1K\n");
#if 0
	fprintf(fp, "  -l, --local           limit listing to local file systems\n");
	fprintf(fp, "      --no-sync         do not invoke sync before getting usage info (default)\n");
	fprintf(fp, "      --output[=FIELD_LIST]  use the output format defined by FIELD_LIST,\n");
	fprintf(fp, "                               or print all fields if FIELD_LIST is omitted.\n");
#endif
	fprintf(fp, "  -P, --portability     use the POSIX output format\n");
#if 0
	fprintf(fp, "      --sync            invoke sync before getting usage info\n");
	fprintf(fp, "  -t, --type=TYPE       limit listing to file systems of type TYPE\n");
#endif
	fprintf(fp, "  -T, --print-type      print file system type\n");
#if 0
	fprintf(fp, "  -x, --exclude-type=TYPE   limit listing to file systems not of type TYPE\n");
#endif
	fprintf(fp, "      --help            display this help and exit\n");
	fprintf(fp, "      --version         output version information and exit\n");
#if 0
	fprintf(fp, "\n");
	fprintf(fp, "Display values are in units of the first available SIZE from --block-size,\n");
	fprintf(fp, "and the DF_BLOCK_SIZE, BLOCK_SIZE and BLOCKSIZE environment variables.\n");
	fprintf(fp, "Otherwise, units default to 1024 bytes (or 512 if POSIXLY_CORRECT is set).\n");
	fprintf(fp, "\n");
	fprintf(fp, "SIZE is an integer and optional unit (example: 10M is 10*1024*1024).  Units\n");
	fprintf(fp, "are K, M, G, T, P, E, Z, Y (powers of 1024) or KB, MB, ... (powers of 1000).\n");
	fprintf(fp, "\n");
	fprintf(fp, "FIELD_LIST is a comma-separated list of columns to be included.  Valid\n");
	fprintf(fp, "field names are: 'source', 'fstype', 'itotal', 'iused', 'iavail', 'ipcent',\n");
	fprintf(fp, "'size', 'used', 'avail', 'pcent' and 'target' (see info page).\n");
#endif
}

static void version(FILE* fp, const char* argv0)
{
	fprintf(fp, "%s (Sortix) %s\n", argv0, VERSIONSTR);
}

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "");

	bool all = false;
	struct display_format format;
	memset(&format, 0, sizeof(format));
	format.magformat = MAGFORMAT_DEFAULT;
	bool total = false;

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
			case 'a': all = true; break;
			case 'h': format.magformat = MAGFORMAT_HUMAN_READABLE; break;
			case 'H': format.magformat = MAGFORMAT_SI; break;
			case 'i': format.inodes = true; break;
			case 'k': format.magformat = MAGFORMAT_1024B; break;
			case 'P': format.portability = true; break;
			case 'T': format.print_type = true; break;
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
			all = true;
		else if ( !strcmp(arg, "--human-readable") )
			format.magformat = MAGFORMAT_HUMAN_READABLE;
		else if ( !strcmp(arg, "--inodes") )
			format.inodes = true;
		else if ( !strcmp(arg, "--portability") )
			format.portability = true;
		else if ( !strcmp(arg, "--print-type") )
			format.print_type = true;
		else if ( !strcmp(arg, "--si") )
			format.magformat = MAGFORMAT_SI;
		else if ( !strcmp(arg, "--total") )
			total = true;
		else
		{
			fprintf(stderr, "%s: unknown option: %s\n", argv0, arg);
			help(stderr, argv0);
			exit(1);
		}
	}

	compact_arguments(&argc, &argv);

	if ( format.magformat == MAGFORMAT_DEFAULT )
		format.magformat = format.portability || format.inodes ?
		                   MAGFORMAT_512B : MAGFORMAT_HUMAN_READABLE;

	bool success = true;

	char** paths = argv + 1;
	size_t paths_count = argc - 1;

	if ( argc == 1 )
	{
		(void) all;
		get_mounts(&paths, &paths_count);
		qsort(paths, paths_count, sizeof(char*), strcmp_indirect);
	}

	struct df* dfs = (struct df*) reallocarray(NULL, paths_count + 1, sizeof(struct df));
	if ( !dfs )
		error(1, errno, "malloc");
	for ( size_t i = 0; i < paths_count; i++ )
	{
		if ( !df(paths[i], &dfs[i]) )
			success = false;
	}
	if ( total )
		dftotal(&dfs[paths_count], dfs, paths_count);
	display(dfs, paths_count + (total ? 1 : 0), &format);
	for ( size_t i = 0; i < paths_count; i++ )
		dffree(&dfs[i]);
	if ( total )
		dffree(&dfs[paths_count]);
	free(dfs);

	if ( ferror(stdout) || fflush(stdout) == EOF )
		success = false;

	return success ? 0 : 1;
}
