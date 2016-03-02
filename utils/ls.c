/*
 * Copyright (c) 2011, 2012, 2013, 2014, 2015 Jonas 'Sortie' Termansen.
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
 * ls.c
 * Lists directory contents.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <assert.h>
#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <libgen.h>
#include <locale.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <wchar.h>

struct passwd* getpwuid_cache(uid_t uid)
{
	static struct passwd* cache_pwd;
	static uid_t cache_uid;
	if ( cache_pwd && cache_uid == uid )
		return cache_pwd;
	if ( (cache_pwd = getpwuid(uid)) )
		cache_uid = uid;
	return cache_pwd;
}

struct group* getgrgid_cache(gid_t gid)
{
	static struct group* cache_grp;
	static gid_t cache_gid;
	if ( cache_grp && cache_gid == gid )
		return cache_grp;
	if ( (cache_grp = getgrgid(gid)) )
		cache_gid = gid;
	return cache_grp;
}

enum dotfile
{
	DOTFILE_NO,
	DOTFILE_ALMOST,
	DOTFILE_ALL,
};

enum sort
{
	SORT_COLLATE,
	SORT_SIZE,
	SORT_TIME,
};

enum timestamp
{
	TIMESTAMP_ATIME,
	TIMESTAMP_CTIME,
	TIMESTAMP_MTIME,
};

struct record
{
	struct dirent* dirent;
	struct stat st;
	int stat_attempt;
	char* symlink_path;
	struct stat symlink_st;
	int symlink_stat_attempt;
};

static int order_normal(int comparison)
{
	return comparison;
}

static int order_reverse(int comparison)
{
	return comparison == 0 ? 0 : comparison < 0 ? 1 : -1;
}

static int current_year;

static enum dotfile option_all = DOTFILE_NO;
static bool option_colors = false;
static bool option_column = false;
static bool option_directory = false;
static bool option_human_readable = false;
static bool option_inode = false;
static bool option_long = false;
static bool option_multiple_operands = false;
static bool option_recursive = false;
static int (*option_reverse)(int) = order_normal;
static enum sort option_sort = SORT_COLLATE;
static enum timestamp option_time_type = TIMESTAMP_MTIME;

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

static void print_left_aligned(const char* string, size_t field_width)
{
	size_t string_width = string_display_length(string);
	fputs(string, stdout);
	for ( size_t i = string_width; i < field_width; i++ )
		putchar(' ');
}

static void print_right_aligned(const char* string, size_t field_width)
{
	size_t string_width = string_display_length(string);
	for ( size_t i = string_width; i < field_width; i++ )
		putchar(' ');
	fputs(string, stdout);
}

#define FORMAT_BYTES_LENGTH (sizeof(off_t) * 3 + 1 + 1 + 1)
static void format_bytes_amount(char* dest, size_t destsize, uintmax_t num_bytes)
{
	uintmax_t value = num_bytes;
	uintmax_t value_fraction = 0;
	uintmax_t exponent = 1024;
	char suffixes[] = { 'B', 'K', 'M', 'G', 'T', 'P', 'E', 'Z', 'Y' };
	size_t num_suffixes = sizeof(suffixes) / sizeof(suffixes[0]);
	size_t suffix_index = 0;
	while ( exponent <= value && suffix_index + 1 < num_suffixes)
	{
		value_fraction = value % exponent;
		value /= exponent;
		suffix_index++;
	}
	char suffix = suffixes[suffix_index];
	if ( suffix_index == 0 )
	{
		snprintf(dest, destsize, "%ju%c", value, suffix);
		return;
	}
	char value_fraction_char = '0' + (value_fraction / (1024 / 10 + 1)) % 10;
	snprintf(dest, destsize, "%ju.%c%c", value, value_fraction_char, suffix);
}

static struct dirent* dirent_dup(struct dirent* entry)
{
	size_t size = sizeof(struct dirent) + strlen(entry->d_name) + 1;
	struct dirent* copy = (struct dirent*) malloc(size);
	if ( !copy )
		return NULL;
	memcpy(copy, entry, size);
	return copy;
}

static struct dirent* dirent_make(const char* name)
{
	size_t size = sizeof(struct dirent) + strlen(name) + 1;
	struct dirent* ret = (struct dirent*) malloc(size);
	if ( !ret )
		return NULL;
	strcpy(ret->d_name, name);
	return ret;
}

static struct timespec record_timestamp(struct record* record)
{
	switch ( option_time_type )
	{
	case TIMESTAMP_ATIME: return record->st.st_atim;
	case TIMESTAMP_CTIME: return record->st.st_ctim;
	case TIMESTAMP_MTIME: return record->st.st_mtim;
	}
	__builtin_unreachable();
}

static int sort_records(const void* a_ptr, const void* b_ptr)
{
	struct record* a = (struct record*) a_ptr;
	struct record* b = (struct record*) b_ptr;
	if ( option_sort == SORT_SIZE )
	{
		if ( a->st.st_size < b->st.st_size )
			return option_reverse(1);
		if ( b->st.st_size < a->st.st_size )
			return option_reverse(1);
	}
	else if ( option_sort == SORT_TIME )
	{
		struct timespec a_ts = record_timestamp(a);
		struct timespec b_ts = record_timestamp(b);
		if ( a_ts.tv_sec < b_ts.tv_sec )
			return option_reverse(1);
		if ( a_ts.tv_sec > b_ts.tv_sec )
			return option_reverse(-1);
		if ( a_ts.tv_nsec < b_ts.tv_nsec )
			return option_reverse(1);
		if ( a_ts.tv_nsec > b_ts.tv_nsec )
			return option_reverse(-1);
	}
	return option_reverse(strcoll(a->dirent->d_name, b->dirent->d_name));
}

static int sort_files_then_dirs(const void* a_ptr, const void* b_ptr)
{
	struct record* a = (struct record*) a_ptr;
	struct record* b = (struct record*) b_ptr;
	if ( !option_directory )
	{
		if ( S_ISDIR(a->st.st_mode) && !S_ISDIR(b->st.st_mode) )
			return 1;
		if ( !S_ISDIR(a->st.st_mode) && S_ISDIR(b->st.st_mode) )
			return -1;
	}
	return sort_records(a_ptr, b_ptr);
}

static unsigned char mode_to_dt(mode_t mode)
{
	if ( S_ISBLK(mode) )
		return DT_BLK;
	if ( S_ISCHR(mode) )
		return DT_CHR;
	if ( S_ISDIR(mode) )
		return DT_DIR;
	if ( S_ISFIFO(mode) )
		return DT_FIFO;
	if ( S_ISLNK(mode) )
		return DT_LNK;
	if ( S_ISREG(mode) )
		return DT_REG;
	if ( S_ISSOCK(mode) )
		return DT_SOCK;
	return DT_UNKNOWN;
}

static void color(const char** pre,
                  const char** post,
                  struct stat* st,
                  bool failure)
{
	*pre = "";
	*post = "";

	if ( !option_colors )
		return;

	if ( failure )
	{
		*pre = "\e[31m";
		*post = "\e[m";
		return;
	}

	*post = "\e[m";
	switch ( mode_to_dt(st->st_mode) )
	{
	case DT_UNKNOWN: *pre = "\e[91m"; break;
	case DT_BLK: *pre = "\e[93m"; break;
	case DT_CHR: *pre = "\e[93m"; break;
	case DT_DIR: *pre = "\e[36m"; break;
	case DT_FIFO: *pre = "\e[33m"; break;
	case DT_LNK: *pre = "\e[96m"; break;
	case DT_SOCK: *pre = "\e[35m"; break;
	case DT_REG:
		if ( st->st_mode & 0111 )
			*pre = "\e[32m";
		else
			*post = "";
		break;
	default:
		*post = "";
	}
}

static void color_record(const char** pre,
                         const char** post,
                         struct record* record)
{
	bool failure = record->stat_attempt < 0 ||
	               (record->dirent->d_type == DT_LNK && !record->symlink_path);
	color(pre, post, &record->st, failure);
}

static void color_symlink(const char** pre,
                        const char** post,
                        struct record* record)
{
	color(pre, post, &record->symlink_st, record->symlink_stat_attempt < 0);
}

static bool should_stat(struct record* record)
{
	(void) record;
	return option_colors ||
	       option_long ||
	       option_recursive ||
	       option_sort != SORT_COLLATE;
}

static bool stat_symlink(DIR* dir, const char* dirpath, struct record* record)
{
	if ( !(0 < record->st.st_size && record->st.st_size <= 65536) )
		return true;
	size_t symlink_path_size = record->st.st_size + 1;
	if ( !(record->symlink_path = (char*) malloc(symlink_path_size)) )
		err(1, "malloc");
	ssize_t ret = readlinkat(dirfd(dir), record->dirent->d_name,
	                         record->symlink_path, symlink_path_size);
	if ( ret < 0 )
	{
		warn("readlink: %s/%s", dirpath, record->dirent->d_name);
		return false;
	}
	record->symlink_path[ret] = '\0';
	if ( fstatat(dirfd(dir), record->symlink_path, &record->symlink_st, 0) < 0 )
		return record->symlink_stat_attempt = -1, true;
	return record->symlink_stat_attempt = 1, true;
}

static bool stat_record(DIR* dir, const char* dirpath, struct record* record)
{
	record->st.st_ino = record->dirent->d_ino;
	record->st.st_dev = record->dirent->d_dev;
	if ( !should_stat(record) )
		return record->stat_attempt = 0, true;
	if ( fstatat(dirfd(dir), record->dirent->d_name, &record->st,
	             AT_SYMLINK_NOFOLLOW) < 0 )
	{
		warn("stat: %s/%s", dirpath, record->dirent->d_name);
		return record->stat_attempt = -1, false;
	}
	if ( record->dirent->d_type == DT_UNKNOWN )
		record->dirent->d_type = mode_to_dt(record->st.st_mode);
	record->stat_attempt = 1;
	if ( S_ISLNK(record->st.st_mode) && !stat_symlink(dir, dirpath, record) )
		return false;
	return true;
}

static void show_simple(struct record* records, size_t count)
{
	for ( size_t i = 0; i < count; i++ )
	{
		struct record* record = &records[i];
		if ( option_inode )
			printf("%ju ", (uintmax_t) record->st.st_ino);
		const char* pre;
		const char* post;
		color_record(&pre, &post, record);
		printf("%s%s%s\n", pre, record->dirent->d_name, post);
	}
}

struct column_size
{
	size_t width_inode;
	size_t width_name;
};

static void show_column(struct record* records, size_t count)
{
	static size_t display_width = 80;
	static bool display_width_set = false;
	if ( !display_width_set )
	{
		const char* display_width_env = getenv("COLUMNS");
		struct winsize ws;
		if ( display_width_env )
			display_width = strtoul(display_width_env, NULL, 10);
		else if ( tcgetwinsize(1, &ws) == 0 )
			display_width = ws.ws_col;
	}

	if ( !count )
		return;

	// TODO: -x support.
	struct column_size* column_sizes;
	size_t columns = 0;
	size_t rows = 0;
	while ( true )
	{
		size_t attempt_rows = rows + 1;
		size_t attempt_columns = count / attempt_rows +
		                         (count % attempt_rows ? 1 : 0);
		struct column_size* attempt_column_sizes = (struct column_size*)
			reallocarray(NULL, attempt_columns, sizeof(struct column_size));
		if ( !attempt_column_sizes )
			err(1, "malloc");
		size_t attempt_width = 0;
		for ( size_t c = 0; c < attempt_columns; c++ )
		{
			struct column_size* c_sizes = &attempt_column_sizes[c];
			memset(c_sizes, 0, sizeof(*c_sizes));
			size_t c_off = attempt_rows * c;
			for ( size_t r = 0; r < attempt_rows; r++ )
			{
				size_t i = c_off + r;
				if ( count <= i )
					break;
				struct record* record = &records[i];
				if ( option_inode )
				{
					char inode_str[sizeof(ino_t) * 3];
					snprintf(inode_str, sizeof(inode_str),
					         "%ju", (uintmax_t) record->dirent->d_ino);
					size_t inode_width = string_display_length(inode_str);
					if ( c_sizes->width_inode < inode_width )
						c_sizes->width_inode = inode_width;
				}
				const char* name = record->dirent->d_name;
				size_t name_width = string_display_length(name);
				if ( c_sizes->width_name < name_width )
					c_sizes->width_name = name_width;
			}
			if ( c != 0 )
				attempt_width += 2;
			if ( option_inode )
				attempt_width += c_sizes->width_inode + 1;
			attempt_width += c_sizes->width_name;
		}
		rows = attempt_rows;
		columns = attempt_columns;
		if ( attempt_width <= display_width || attempt_rows == count )
		{
			column_sizes = attempt_column_sizes;
			break;
		}
		free(attempt_column_sizes);
	}

	for ( size_t r = 0; r < rows; r++ )
	{
		for ( size_t c = 0; c < columns; c++ )
		{
			size_t i = c * rows + r;
			if ( count <= i )
				break;
			struct column_size* c_sizes = &column_sizes[c];
			struct record* record = &records[i];
			if ( option_inode )
			{
				char inode_str[sizeof(ino_t) * 3];
				snprintf(inode_str, sizeof(inode_str),
					     "%ju", (uintmax_t) record->dirent->d_ino);
				print_right_aligned(inode_str, c_sizes->width_inode);
				putchar(' ');
			}
			const char* pre;
			const char* post;
			color_record(&pre, &post, record);
			fputs(pre, stdout);
			if ( c + 1 == columns || count - i <= rows )
				fputs(record->dirent->d_name, stdout);
			else
				print_left_aligned(record->dirent->d_name, c_sizes->width_name);
			fputs(post, stdout);
			if ( c + 1 == columns || count - i <= rows )
				putchar('\n');
			else
				fputs("  ", stdout);
		}
	}

	free(column_sizes);
}

static void show_long(struct record* records, size_t count)
{
	size_t nlink_field_width = 0;
	size_t owner_field_width = 0;
	size_t group_field_width = 0;
	size_t size_field_width = 0;
	size_t time_field_width = 0;

	for ( size_t i = 0; i < count; i++ )
	{
		struct record* record = &records[i];

		nlink_t nlink = record->st.st_nlink;
		char nlink_str[sizeof(nlink_t) * 3];
		snprintf(nlink_str, sizeof(nlink_str), "%ju", (uintmax_t) nlink);
		size_t nlink_width = string_display_length(nlink_str);
		if ( nlink_field_width < nlink_width )
			nlink_field_width = nlink_width;

		char owner_fallback[sizeof(uid_t) * 3];
		struct passwd* pwd;
		const char* owner_str;
		if ( record->stat_attempt != 1 )
			owner_str = "?";
		else if ( (pwd = getpwuid_cache(record->st.st_uid)) )
			owner_str = pwd->pw_name;
		else
		{
			snprintf(owner_fallback, sizeof(owner_fallback),
			         "%ju", (uintmax_t) record->st.st_uid);
			owner_str = owner_fallback;
		}
		size_t owner_width = string_display_length(owner_str);
		if ( owner_field_width < owner_width )
			owner_field_width = owner_width;

		char group_fallback[sizeof(gid_t) * 3];
		struct group* grp;
		const char* group_str;
		if ( record->stat_attempt != 1 )
			group_str = "?";
		else if ( (grp = getgrgid_cache(record->st.st_gid)) )
			group_str = grp->gr_name;
		else
		{
			snprintf(group_fallback, sizeof(group_fallback),
			         "%ju", (uintmax_t) record->st.st_gid);
			group_str = group_fallback;
		}
		size_t group_width = string_display_length(group_str);
		if ( group_field_width < group_width )
			group_field_width = group_width;

		off_t size = record->st.st_size;
		char size_str[FORMAT_BYTES_LENGTH + 1];
		if ( option_human_readable )
			format_bytes_amount(size_str, sizeof(size_str), size);
		else
			snprintf(size_str, sizeof(size_str), "%ji", (intmax_t) size);
		size_t size_width = string_display_length(size_str);
		if ( size_field_width < size_width )
			size_field_width = size_width;

		struct timespec ts = record_timestamp(record);
		struct tm mod_tm;
		localtime_r(&ts.tv_sec, &mod_tm);
		char time_str[64];
		if ( current_year == mod_tm.tm_year )
			strftime(time_str, sizeof(time_str), "%b %e %H:%M", &mod_tm);
		else
			strftime(time_str, sizeof(time_str), "%b %e  %Y", &mod_tm);
		size_t time_width = string_display_length(time_str);
		if ( time_field_width < time_width )
			time_field_width = time_width;
	}

	// TODO: Show a total number of filesystem blocks.
	//       (But only when listing a directory?)

	for ( size_t i = 0; i < count; i++ )
	{
		struct record* record = &records[i];

		if ( option_inode )
			printf("%ju ", (uintmax_t) record->st.st_ino);

		mode_t mode = record->st.st_mode;
		char perms[11];
		switch ( record->dirent->d_type )
		{
		case DT_UNKNOWN: perms[0] = '?'; break;
		case DT_BLK: perms[0] = 'b'; break;
		case DT_CHR: perms[0] = 'c'; break;
		case DT_DIR: perms[0] = 'd'; break;
		case DT_FIFO: perms[0] = 'p'; break;
		case DT_LNK: perms[0] = 'l'; break;
		case DT_REG: perms[0] = '-'; break;
		case DT_SOCK: perms[0] = 's'; break;
		default: perms[0] = '?'; break;
		}
		const char flagnames[] = { 'x', 'w', 'r' };
		for ( size_t n = 0; n < 9; n++ )
			perms[9-n] = mode & (1 << n) ? flagnames[n % 3] : '-';
		if ( mode & S_ISUID )
			perms[3] = mode & 0100 ? 's' : 'S';
		if ( mode & S_ISGID )
			perms[6] = mode & 0010 ? 's' : 'S';
		if ( mode & S_ISVTX )
			perms[9] = mode & 0001 ? 't' : 'T';
		if ( record->stat_attempt != 1 )
			memset(perms, '?', sizeof(perms) - 1);
		perms[10] = '\0';
		fputs(perms, stdout);
		putchar(' ');

		nlink_t nlink = record->st.st_nlink;
		char nlink_str[sizeof(nlink_t) * 3];
		snprintf(nlink_str, sizeof(nlink_str), "%ju", (uintmax_t) nlink);
		print_right_aligned(nlink_str, nlink_field_width);
		putchar(' ');

		char owner_fallback[sizeof(uid_t) * 3];
		struct passwd* pwd;
		const char* owner_str;
		if ( record->stat_attempt != 1 )
			owner_str = "?";
		else if ( (pwd = getpwuid_cache(record->st.st_uid)) )
			owner_str = pwd->pw_name;
		else
		{
			snprintf(owner_fallback, sizeof(owner_fallback),
			         "%ju", (uintmax_t) record->st.st_uid);
			owner_str = owner_fallback;
		}
		print_left_aligned(owner_str, owner_field_width);
		putchar(' ');

		char group_fallback[sizeof(gid_t) * 3];
		struct group* grp;
		const char* group_str;
		if ( record->stat_attempt != 1 )
			group_str = "?";
		else if ( (grp = getgrgid_cache(record->st.st_gid)) )
			group_str = grp->gr_name;
		else
		{
			snprintf(group_fallback, sizeof(group_fallback),
			         "%ju", (uintmax_t) record->st.st_gid);
			group_str = group_fallback;
		}
		print_left_aligned(group_str, group_field_width);
		putchar(' ');

		off_t size = record->st.st_size;
		char size_str[FORMAT_BYTES_LENGTH + 1];
		if ( option_human_readable )
			format_bytes_amount(size_str, sizeof(size_str), size);
		else
			snprintf(size_str, sizeof(size_str), "%ji", (intmax_t) size);
		print_right_aligned(size_str, size_field_width);
		putchar(' ');

		struct timespec ts = record_timestamp(record);
		struct tm mod_tm;
		localtime_r(&ts.tv_sec, &mod_tm);
		char time_str[64];
		if ( current_year == mod_tm.tm_year )
			strftime(time_str, sizeof(time_str), "%b %e %H:%M", &mod_tm);
		else
			strftime(time_str, sizeof(time_str), "%b %e  %Y", &mod_tm);
		print_left_aligned(time_str, time_field_width);
		putchar(' ');

		const char* pre;
		const char* post;
		color_record(&pre, &post, record);
		fputs(pre, stdout);
		fputs(record->dirent->d_name, stdout);
		fputs(post, stdout);
		if ( record->symlink_path )
		{
			color_symlink(&pre, &post, record);
			fputs(" -> ", stdout);
			fputs(pre, stdout);
			fputs(record->symlink_path, stdout);
			fputs(post, stdout);
		}
		putchar('\n');
	}
}

static void show(struct record* records, size_t count)
{
	qsort(records, count, sizeof(*records), sort_records);
	if ( option_long )
		show_long(records, count);
	else if ( option_column )
		show_column(records, count);
	else
		show_simple(records, count);
}

static int ls_directory(int parentfd, const char* relpath, const char* path);

static int show_recursive(int fd, const char* path,
                          struct record* records, size_t count)
{
	// TODO: Use a proper join path function below.
	if ( path && !strcmp(path, "/") )
		path = "";
	if ( option_recursive && path )
		show(records, count);
	int ret = 0;
	qsort(records, count, sizeof(*records), sort_files_then_dirs);
	size_t nondir_count = count;
	if ( !option_directory )
	{
		for ( nondir_count = 0; nondir_count < count; nondir_count++ )
			if ( S_ISDIR(records[nondir_count].st.st_mode) )
				break;
	}
	if ( !(option_recursive && path) )
		show(records, nondir_count);
	for ( size_t i = nondir_count; i < count; i++ )
	{
		struct record* record = &records[i];
		static bool not_first_directory = false;
		if ( (not_first_directory && option_recursive) || i != 0 )
			putchar('\n');
		not_first_directory = true;
		const char* name = record->dirent->d_name;
		char* subpath_storage = NULL;
		if ( path && asprintf(&subpath_storage, "%s/%s", path, name) < 0 )
			err(1, "malloc");
		const char* subpath = path ? subpath_storage : name;
		if ( option_recursive || 2 <= count )
			printf("%s:\n", subpath);
		ret |= ls_directory(fd, name, subpath);
		free(subpath_storage);
	}
	return ret;
}

static int ls_directory(int parentfd, const char* relpath, const char* path)
{
	int fd = openat(parentfd, relpath, O_RDONLY | O_DIRECTORY);
	if ( fd < 0 )
	{
		warn("%s", path);
		return 1;
	}
	DIR* dir = fdopendir(fd);
	if ( !dir )
	{
		warn("fdopendir: %s", path);
		close(fd);
		return 1;
	}

	size_t records_used = 0;
	size_t records_length = 64;
	struct record* records = (struct record*)
		reallocarray(NULL, records_length, sizeof(struct record));
	if ( !records )
		err(1, "malloc");

	int ret = 0;

	struct dirent* entry;
	while ( (errno = 0, entry = readdir(dir)) )
	{
		const char* name = entry->d_name;
		bool isdotdot = strcmp(name, ".") == 0 || strcmp(name, "..") == 0;
		if ( isdotdot && option_all != DOTFILE_ALL )
			continue;
		bool isdotfile = !isdotdot && name[0] == '.';
		if ( isdotfile && option_all == DOTFILE_NO )
			continue;

		if ( records_used == records_length )
		{
			struct record* new_records = (struct record*)
				reallocarray(records, records_length, 2 * sizeof(struct record));
			if ( !new_records )
				err(1, "malloc");
			records = new_records;
			records_length *= 2;
		}

		struct record* record = &records[records_used++];
		memset(record, 0, sizeof(*record));
		if ( !(record->dirent = dirent_dup(entry)) )
			err(1, "malloc");
		if ( stat_record(dir, path, record) < 0 )
			ret = 1;
	}

	// TODO: Stupid /dev/net/fs fake directory, so ignore ENOTDIR for now.
	if ( errno != 0 && errno != ENOTDIR )
		err(1, "%s", path);

	if ( option_recursive )
		show_recursive(dirfd(dir), path, records, records_used);
	else
		show(records, records_used);

	closedir(dir);

	for ( size_t i = 0; i < records_used; i++ )
	{
		free(records[i].dirent);
		free(records[i].symlink_path);
	}
	free(records);

	return ret;
}

static int ls_operands(char** paths, size_t paths_count)
{
	int ret = 0;
	struct record* records = (struct record*)
		reallocarray(NULL, paths_count, sizeof(struct record));
	if ( !records )
		err(1, "malloc");
	size_t records_used = 0;
	for ( size_t i = 0; i < paths_count; i++ )
	{
		struct record* record = &records[records_used];
		memset(record, 0, sizeof(*record));
		const char* path = paths[i];
		if ( fstatat(AT_FDCWD, path, &record->st, AT_SYMLINK_NOFOLLOW) < 0 )
		{
			warn("%s", path);
			ret = 1;
			continue;
		}
		record->stat_attempt = 1;
		if ( !(record->dirent = dirent_make(paths[i])) )
			err(1, "malloc");
		record->dirent->d_ino = record->st.st_ino;
		record->dirent->d_dev = record->st.st_dev;
		record->dirent->d_type = mode_to_dt(record->st.st_mode);
		records_used++;
	}
	show_recursive(AT_FDCWD, NULL, records, records_used);
	for ( size_t i = 0; i < records_used; i++ )
		free(records[i].dirent);
	free(records);
	return ret;
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

static void version(FILE* fp, const char* argv0)
{
	fprintf(fp, "%s (Sortix) %s\n", argv0, VERSIONSTR);
}

static void help(FILE* fp, const char* argv0)
{
	fprintf(fp, "Usage: %s [OPTION]... [FILE]...\n", argv0);
}

int main(int argc, char** argv)
{
	setlocale(LC_ALL, "");

	if ( isatty(1) )
	{
		option_colors = true;
		option_column = true;
	}

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
			case '1': option_column = false; break; // TODO: Semantics?
			case 'a': option_all = DOTFILE_ALL; break;
			case 'A': option_all = DOTFILE_ALMOST; break;
			case 'c': option_time_type = TIMESTAMP_CTIME; break;
			case 'C': option_column = true; break; // TODO: Disable -l and such.
			case 'd': option_directory = true; break;
			// TODO: -f.
			// TODO: -F.
			case 'h': option_human_readable = true; break;
			// TODO: -H.
			case 'i': option_inode = true; break;
			// TODO: -k.
			case 'l': option_long = true; break;
			// TODO: -L.
			// TODO: -m.
			// TODO: -n.
			// TODO: -p.
			// TODO: -q.
			case 'r': option_reverse = order_reverse; break;
			case 'R': option_recursive = true; break;
			// TODO: -s.
			case 'S': option_sort = SORT_SIZE; break;
			case 't': option_sort = SORT_TIME; break;
			case 'u': option_time_type = TIMESTAMP_ATIME; break;
			// TODO: -x.
			default:
				fprintf(stderr, "%s: unknown option -- '%c'\n", argv0, c);
				help(stderr, argv0);
				exit(1);
			}
		}
		else if ( !strcmp(arg, "--version") )
			version(stdout, argv0), exit(0);
		else if ( !strcmp(arg, "--help") )
			help(stdout, argv0), exit(0);
		else if ( !strcmp(arg, "--all") )
			option_all = DOTFILE_ALL;
		else if ( !strcmp(arg, "--almost-all") )
			option_all = DOTFILE_ALMOST;
		else if ( !strcmp(arg, "--color=always") ) // TODO: Proper parsing.
			option_colors = true;
		else if ( !strcmp(arg, "--color=auto") ) // TODO: Proper parsing.
			;
		else if ( !strcmp(arg, "--color=never") ) // TODO: Proper parsing.
			option_colors=false;
		else if ( !strcmp(arg, "--directory") )
			option_directory = true;
		else if ( !strcmp(arg, "--human-readable") )
			option_human_readable = true;
		else if ( !strcmp(arg, "--inode") )
			option_inode = true;
		else if ( !strcmp(arg, "--recursive") )
			option_recursive = true;
		else if ( !strcmp(arg, "--reverse") )
			option_reverse = order_reverse;
		else
		{
			fprintf(stderr, "%s: unrecognized option: %s\n", argv0, arg);
			help(stderr, argv0);
			exit(1);
		}
	}

	compact_arguments(&argc, &argv);

	time_t current_time;
	struct tm current_year_tm;
	time(&current_time);
	localtime_r(&current_time, &current_year_tm);
	current_year = current_year_tm.tm_year;

	option_multiple_operands = 3 <= argc;
	char* curdir = (char*) ".";
	int ret = 2 <= argc ? ls_operands(argv + 1, argc - 1)
	                    : ls_operands(&curdir, 1);
	if ( ferror(stdout) || fflush(stdout) == EOF )
		return 1;
	return ret;
}
