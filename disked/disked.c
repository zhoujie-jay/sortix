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

    disked.c
    Disk editor.

*******************************************************************************/

#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <endian.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <fstab.h>
#include <inttypes.h>
#include <ioleast.h>
#include <libgen.h>
#include <locale.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <wchar.h>

#include <fsmarshall.h>

#include <mount/biosboot.h>
#include <mount/blockdevice.h>
#include <mount/devices.h>
#include <mount/ext2.h>
#include <mount/filesystem.h>
#include <mount/gpt.h>
#include <mount/harddisk.h>
#include <mount/mbr.h>
#include <mount/partition.h>
#include <mount/uuid.h>

__attribute__((format(printf, 1, 2)))
static char* print_string(const char* format, ...)
{
	va_list ap;
	va_start(ap, format);
	char* ret;
	if ( vasprintf(&ret, format, ap) < 0 )
		ret = NULL;
	va_end(ap);
	return ret;
}

static bool verify_mountpoint(const char* mountpoint)
{
	size_t index = 0;
	if ( mountpoint[index++] != '/' )
		return false;
	while ( mountpoint[index] )
	{
		if ( mountpoint[index] == '.' )
		{
			index++;
			if ( mountpoint[index] == '.' )
				index++;
			if ( !mountpoint[index] || mountpoint[index] == '/' )
				return false;
		}
		while ( mountpoint[index] != '/' )
			index++;
		while ( mountpoint[index] == '/' )
			index++;
	}
	return true;
}

static void simplify_mountpoint(char* mountpoint)
{
	bool slash_pending = false;
	size_t out = 0;
	for ( size_t in = 0; mountpoint[in]; in++ )
	{
		if ( mountpoint[in] == '/' )
		{
			if ( out == 0 )
			{
				mountpoint[out++] = '/';
				while ( mountpoint[in] == '/' )
					in++;
				in--;
				continue;
			}
			slash_pending = true;
			continue;
		}
		if ( slash_pending )
		{
			mountpoint[out++] = '/';
			slash_pending = false;
		}
		mountpoint[out++] = mountpoint[in];
	}
	mountpoint[out] = '\0';
}

static char* format_bytes_amount(uintmax_t num_bytes)
{
	uintmax_t value = num_bytes;
	uintmax_t value_fraction = 0;
	uintmax_t exponent = 1024;
	char prefixes[] = { '\0', 'K', 'M', 'G', 'T', 'P', 'E', 'Z', 'Y' };
	size_t num_prefixes = sizeof(prefixes) / sizeof(prefixes[0]);
	size_t prefix_index = 0;
	while ( exponent <= value && prefix_index + 1 < num_prefixes)
	{
		value_fraction = value % exponent;
		value /= exponent;
		prefix_index++;
	}
	char prefix_str[] = { prefixes[prefix_index],  'i', 'B', '\0' };
	char value_fraction_char = '0' + (value_fraction / (1024 / 10 + 1)) % 10;
	char* result;
	if ( asprintf(&result, "%ju.%c %s", value, value_fraction_char, prefix_str) < 0 )
		return NULL;
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

static void split_arguments(char* cmd,
                            size_t* argc_ptr,
                            char** argv,
                            size_t argc_max)
{
	size_t argc = 0;
	size_t cmd_offset = 0;
	while ( cmd[cmd_offset] )
	{
		while ( cmd[cmd_offset] && isspace((unsigned char) cmd[cmd_offset]) )
		{
			cmd[cmd_offset] = '\0';
			cmd_offset++;
		}
		if ( !cmd[cmd_offset] )
			break;
		char* out = cmd + cmd_offset;
		size_t out_offset = 0;
		if ( argc < argc_max )
			argv[argc++] = out;
		bool escape = false;
		bool quote_single = false;
		bool quote_double = false;
		while ( cmd[cmd_offset] )
		{
			if ( !escape && !quote_single && cmd[cmd_offset] == '\\' )
			{
				cmd_offset++;
				escape = true;
			}
			else if ( !escape && !quote_double && cmd[cmd_offset] == '\'' )
			{
				cmd_offset++;
				quote_single = !quote_single;
			}
			else if ( !escape && !quote_single && cmd[cmd_offset] == '"' )
			{
				cmd_offset++;
				quote_double = !quote_double;
			}
			else if ( !(escape || quote_single || quote_double) &&
				      isspace((unsigned char) cmd[cmd_offset]) )
			{
				break;
			}
			else
			{
				out[out_offset++] = cmd[cmd_offset++];
				escape = false;
			}
		}
		char last_c = cmd[cmd_offset];
		out[out_offset] = '\0';
		if ( !last_c )
			break;
		cmd[cmd_offset++] = '\0';
	}
	*argc_ptr = argc;
}

static const char* device_name(const char* name)
{
	if ( !strncmp(name, "/dev/", strlen("/dev/")) )
		return name + strlen("/dev/");
	return name;
}

static void display_rows_columns(char* (*format)(void*, size_t, size_t),
                                 void* ctx, size_t rows, size_t columns)
{
	size_t* widths = (size_t*) reallocarray(NULL, sizeof(size_t), columns);
	assert(widths);
	for ( size_t c = 0; c < columns; c++ )
		widths[c] = 0;
	for ( size_t r = 0; r < rows; r++ )
	{
		for ( size_t c = 0; c < columns; c++ )
		{
			char* entry = format(ctx, r, c);
			assert(entry);
			size_t width = string_display_length(entry);
			if ( widths[c] < width )
				widths[c] = width;
			free(entry);
		}
	}
	for ( size_t r = 0; r < rows; r++ )
	{
		for ( size_t c = 0; c < columns; c++ )
		{
			char* entry = format(ctx, r, c);
			assert(entry);
			size_t width = string_display_length(entry);
			printf("%s", entry);
			if ( c + 1 != columns )
			{
				for ( size_t i = width; i < widths[c]; i++ )
					putchar(' ');
				printf("  ");
			}
			free(entry);
		}
		printf("\n");
	}
	free(widths);
}

static void text(const char* str)
{
	fflush(stdout);
	struct winsize ws;
	struct wincurpos wcp;
	if ( tcgetwinsize(1, &ws) < 0 || tcgetwincurpos(1, &wcp) < 0 )
	{
		printf("%s", str);
		return;
	}
	size_t columns = ws.ws_col;
	size_t column = wcp.wcp_col;
	bool blank = false;
	while ( str[0] )
	{
		if ( str[0] == '\e' )
		{
			size_t length = 1;
			while ( str[length] == '[' ||
			        str[length] == ';' ||
			        ('0' <= str[length] && str[length] <= '9') )
				length++;
			if ( 64 <= str[length] && str[length] <= 126 )
				length++;
			fwrite(str, 1, length, stdout);
			str += length;
			continue;
		}
		else if ( str[0] == '\n' )
		{
			putchar('\n');
			blank = false;
			column = 0;
			str++;
			continue;
		}
		else if ( isblank((unsigned char) str[0]) )
		{
			blank = true;
			str++;
			continue;
		}
		size_t word_length = 0;
		size_t word_columns = 0;
		mbstate_t ps = { 0 };
		while ( str[word_length] &&
		        str[word_length] != '\n' &&
		        !isblank((unsigned char) str[word_length]) )
		{
			wchar_t wc;
			size_t amount = mbrtowc(&wc, str + word_length, SIZE_MAX, &ps);
			if ( amount == (size_t) -2 )
				 break;
			if ( amount == (size_t) -1 )
			{
				memset(&ps, 0, sizeof(ps));
				amount = 1;
			}
			if ( amount == (size_t) 0 )
				break;
			word_length += amount;
			int width = wcwidth(wc);
			if ( width < 0 )
				continue;
			word_columns += width;
		}
		if ( (column && blank ? 1 : 0) + word_columns <= columns - column )
		{
			if ( column && blank )
			{
				putchar(' ');
				column++;
			}
			blank = false;
			fwrite(str, 1, word_length, stdout);
			column += word_columns;
			if ( column == columns )
				column = 0;
		}
		else
		{
			if ( column != 0 && column != columns )
				putchar('\n');
			column = 0;
			blank = false;
			fwrite(str, 1, word_length, stdout);
			column += word_columns;
			column %= columns;
		}
		str += word_length;
	}
	fflush(stdout);
}

static void textf(const char* format, ...)
{
	va_list ap;
	va_start(ap, format);
	char* str;
	int len = vasprintf(&str, format, ap);
	va_end(ap);
	if ( len < 0 )
	{
		vprintf(format, ap);
		return;
	}
	text(str);
	free(str);
}

static void prompt(char* buffer,
                   size_t buffer_size,
                   const char* question,
                   const char* answer)
{
	while ( true )
	{
		text(question);
		if ( answer )
			printf(" [%s] ", answer);
		else
			printf(" ");
		fflush(stdout);
		fgets(buffer, buffer_size, stdin);
		size_t buffer_length = strlen(buffer);
		if ( buffer_length && buffer[buffer_length-1] == '\n' )
			buffer[--buffer_length] = '\0';
		while ( buffer_length && buffer[buffer_length-1] == ' ' )
			buffer[--buffer_length] = '\0';
		if ( !strcmp(buffer, "") )
		{
			if ( !answer )
				continue;
			strlcpy(buffer, answer, buffer_size);
		}
		break;
	}
}

static bool remove_partition_device(const char* path)
{
	// TODO: Refuse to do this if the partition are currently in use.
	if ( unmount(path, UNMOUNT_NOFOLLOW) < 0 && errno != ENOMOUNT )
	{
		warn("unmount: %s", path);
		return false;
	}
	if ( unlink(path) < 0 )
	{
		warn("unlink: %s", path);
		return false;
	}
	return true;
}

static void remove_partition_devices(const char* path)
{
	const char* name = path;
	for ( size_t i = 0; path[i]; i++ )
		if ( path[i] == '/' )
			name = path + i + 1;
	size_t name_length = strlen(name);
	char* dir_path = strdup(path);
	if ( !dir_path )
	{
		warn("%s", dir_path);
		return; // TODO: Error.
	}
	dirname(dir_path);
	DIR* dir = opendir(dir_path);
	struct dirent* entry;
	while ( (errno = 0, entry = readdir(dir)) )
	{
		if ( strncmp(entry->d_name, name, name_length) != 0 )
			continue;
		if ( entry->d_name[name_length] != 'p' )
			continue;
		bool all_digits = true;
		for ( size_t i = name_length + 1; all_digits && entry->d_name[i]; i++ )
			if ( !('0' <= entry->d_name[i] && entry->d_name[i] <= '9') )
				all_digits = false;
		if ( !all_digits )
			continue;
		// TODO: Refuse to do this if the partitions are currently in use.
		if ( unmountat(dirfd(dir), entry->d_name, UNMOUNT_NOFOLLOW) < 0 &&
		     errno != ENOMOUNT )
		{
			warn("unmount: %s/%s", dir_path, entry->d_name);
			// TODO: Warn/error.
		}
		if ( unlinkat(dirfd(dir), entry->d_name, 0) < 0 )
		{
			warn("unlink: %s/%s", dir_path, entry->d_name);
			// TODO: Warn/error.
		}
		rewinddir(dir);
	}
	if ( errno )
	{
		warn("readdir: %s", dir_path);
		// TODO: Error.
	}
	closedir(dir);
	free(dir_path);
}

static int harddisk_compare_path(const void* a_ptr, const void* b_ptr)
{
	struct harddisk* a = *((struct harddisk**) a_ptr);
	struct harddisk* b = *((struct harddisk**) b_ptr);
	return strcmp(a->path, b->path);
}

struct device_area
{
	off_t start;
	off_t length;
	off_t extended_start;
	off_t ebr_off;
	off_t ebr_move_off;
	struct partition* p;
	char* filesystem;
	char* mountpoint;
	size_t ebr_index;
	size_t ebr_move_index;
	bool inside_extended;
};

static int device_area_compare_start(const void* a_ptr, const void* b_ptr)
{
	const struct device_area* a = (const struct device_area*) a_ptr;
	const struct device_area* b = (const struct device_area*) b_ptr;
	if ( a->start < b->start )
		return -1;
	if ( a->start > b->start )
		return 1;
	return 0;
}

static bool match_fstab_device(const char* device, struct blockdevice* bdev)
{
	if ( strncmp(device, "UUID=", strlen("UUID=")) == 0 )
	{
		device += strlen("UUID=");
		if ( !bdev->fs )
			return false;
		if ( !(bdev->fs->flags & FILESYSTEM_FLAG_UUID) )
			return false;
		if ( !uuid_validate(device) )
			return false;
		unsigned char uuid[16];
		uuid_from_string(uuid, device);
		if ( memcmp(bdev->fs->uuid, uuid, 16) != 0 )
			return false;
		return true;
	}
	else if ( bdev->p && !strcmp(device, bdev->p->path) )
		return true;
	else if ( !bdev->p && bdev->hd && !strcmp(device, bdev->hd->path) )
		return true;
	return false;
}

struct rewrite
{
	FILE* in;
	FILE* out;
	const char* in_path;
	char* out_path;
};

bool rewrite_begin(struct rewrite* rewr, const char* in_path)
{
	memset(rewr, 0, sizeof(*rewr));
	rewr->in_path = in_path;
	if ( !(rewr->in = fopen(rewr->in_path, "r")) )
		return false;
	if ( asprintf(&rewr->out_path, "%s.XXXXXX", in_path) < 0 )
		return fclose(rewr->in), false;
	int out_fd = mkstemp(rewr->out_path);
	if ( out_fd < 0 )
		return free(rewr->out_path), fclose(rewr->in), false;
	if ( !(rewr->out = fdopen(out_fd, "w")) )
		return close(out_fd), free(rewr->out_path), fclose(rewr->in), false;
	return true;
}

void rewrite_abort(struct rewrite* rewr)
{
	fclose(rewr->in);
	fclose(rewr->out);
	unlink(rewr->out_path);
	free(rewr->out_path);
}

bool rewrite_finish(struct rewrite* rewr)
{
	struct stat in_st;
	if ( ferror(rewr->out) || fflush(rewr->out) == EOF )
		return rewrite_abort(rewr), false;
	if ( fstat(fileno(rewr->in), &in_st) < 0 )
		return rewrite_abort(rewr), false;
	mode_t mode = in_st.st_mode & 0777;
	if ( in_st.st_uid != getuid() || in_st.st_gid != getgid() )
		mode &= ~getumask();
	if ( fchmod(fileno(rewr->out), mode) < 0 )
		return rewrite_abort(rewr), false;
	if ( rename(rewr->out_path, rewr->in_path) < 0 )
		return rewrite_abort(rewr), false;
	fclose(rewr->in);
	fclose(rewr->out);
	free(rewr->out_path);
	return true;
}

// TODO: Finish this, add decimal support and protect against overflow.
static bool parse_disk_quantity(off_t* out, const char* string, off_t max)
{
	if ( *string && isspace((unsigned char) *string) )
		string++;
	const char* end;
	bool from_end = false;
	intmax_t value = strtoimax(string, (char**) &end, 10);
	if ( value < 0 )
	{
		// TODO: If the least possible value, overflow.
		value = -value;
		from_end = true;
	}
	string = end;
	if ( *string && isspace((unsigned char) *string) )
		string++;
	if ( *string == '.' )
	{
		string++;
		// TODO: Support this!
		if ( strtoimax(string, (char**) &end, 10) < 0 )
			return false;
		string = end;
	}
	if ( *string == '%' )
	{
		string++;
		if ( 100 < value )
			return false;
		if ( value == 100 )
			value = max;
		else
			value = (max * value) / 100;
	}
	else if ( *string == 'b' || *string == 'B' )
	{
	}
	else if ( *string == 'k' || *string == 'K' )
	{
		string++;
		if ( *string == 'i' )
			string++;
		if ( *string == 'b' || *string == 'B' )
			string++;
		value = value * 1024LL;
	}
	else if ( *string == 'm' || *string == 'M' ||
	          !*string || isspace((unsigned char) *string) )
	{
		if ( *string )
			string++;
		if ( *string == 'i' )
			string++;
		if ( *string == 'b' || *string == 'B' )
			string++;
		value = value * (1024LL * 1024LL);
	}
	else if ( *string == 'g' || *string == 'G' )
	{
		string++;
		if ( *string == 'i' )
			string++;
		if ( *string == 'b' || *string == 'B' )
			string++;
		value = value * (1024LL * 1024LL * 1024LL);
	}
	else if ( *string == 't' || *string == 'T' )
	{
		string++;
		if ( *string == 'i' )
			string++;
		if ( *string == 'b' || *string == 'B' )
			string++;
		value = value * (1024LL * 1024LL * 1024LL * 1024LL);
	}
	else if ( *string == 'p' || *string == 'P' )
	{
		string++;
		if ( *string == 'i' )
			string++;
		if ( *string == 'b' || *string == 'B' )
			string++;
		value = value * (1024LL * 1024LL * 1024LL * 1024LL * 1024LL);
	}
	if ( *string && isspace((unsigned char) *string) )
		string++;
	if ( *string )
		return false;
	if ( max < value )
		return false;
	if ( from_end )
		value = max - value;
	uintmax_t uvalue = value;
	uintmax_t mask = ~(UINTMAX_C(1048576) - 1);
	uvalue = -(-value & mask);
	value = (off_t) uvalue;
	return *out = value, true;
}

static bool interactive;
static bool quitting;
static struct harddisk** hds;
static size_t hds_count;
static struct harddisk* current_hd;
static enum partition_table_type current_pt_type;
static struct partition_table* current_pt;
static size_t current_areas_count;
static struct device_area* current_areas;
static const char* fstab_path = "/etc/fstab";

static bool lookup_fstab_by_blockdevice(struct fstab* out_fsent,
                                        char** out_storage,
                                        struct blockdevice* bdev)
{
	FILE* fstab_fp = fopen(fstab_path, "r");
	if ( !fstab_fp )
		return false;
	char* line = NULL;
	size_t line_size = 0;
	ssize_t line_length;
	while ( 0 <= (errno = 0, line_length = getline(&line, &line_size, fstab_fp)) )
	{
		if ( line_length && line[line_length - 1] == '\n' )
			line[--line_length] = '\0';
		if ( !scanfsent(line, out_fsent) )
			continue;
		if ( match_fstab_device(out_fsent->fs_spec, bdev) )
		{
			*out_storage = line;
			fclose(fstab_fp);
			return true;
		}
	}
	free(line);
	fclose(fstab_fp);
	if ( errno )
		return false;
	return false;
}

static bool remove_blockdevice_from_fstab(struct blockdevice* bdev)
{
	struct rewrite rewr;
	if ( !rewrite_begin(&rewr, fstab_path) )
	{
		if ( errno == ENOENT )
			return true;
		return false;
	}
	char* line = NULL;
	size_t line_size = 0;
	ssize_t line_length;
	while ( 0 <= (errno = 0, line_length = getline(&line, &line_size, rewr.in)) )
	{
		if ( line_length && line[line_length - 1] == '\n' )
			line[--line_length] = '\0';
		char* dup = strdup(line);
		if ( !dup )
			return rewrite_abort(&rewr), false;
		struct fstab fsent;
		if ( !scanfsent(dup, &fsent) ||
		     !match_fstab_device(fsent.fs_spec, bdev) )
			fprintf(rewr.out, "%s\n", line);
		free(dup);
	}
	free(line);
	if ( errno )
		return rewrite_abort(&rewr), false;
	return rewrite_finish(&rewr);
}

static void print_blockdevice_fsent(FILE* fp,
                                    struct blockdevice* bdev,
                                    const char* mountpoint)
{
	char uuid[5 + UUID_STRING_LENGTH + 1];
	const char* spec = bdev->p ? bdev->p->path : bdev->hd->path;
	if ( bdev->fs->flags & FILESYSTEM_FLAG_UUID )
	{
		strcpy(uuid, "UUID=");
		uuid_to_string(bdev->fs->uuid, uuid + 5);
		spec = uuid;
	}
	fprintf(fp, "%s %s %s %s %i %i\n",
	        spec,
	        mountpoint,
	        bdev->fs->fstype_name,
		    "rw",
		    1,
		    !strcmp(mountpoint, "/") ? 1 : 2);
}

static bool add_blockdevice_to_fstab(struct blockdevice* bdev,
                                     const char* mountpoint)
{
	assert(bdev->fs);
	struct rewrite rewr;
	if ( !rewrite_begin(&rewr, fstab_path) )
	{
		if ( errno == ENOENT )
		{
			FILE* fp = fopen(fstab_path, "w");
			if ( !fp )
				return false;
			print_blockdevice_fsent(fp, bdev, mountpoint);
			if ( ferror(fp) || fflush(fp) == EOF )
				return fclose(fp), false;
			fclose(fp);
			return true;
		}
		return false;
	}
	char* line = NULL;
	size_t line_size = 0;
	ssize_t line_length;
	bool found = false;
	while ( 0 <= (errno = 0, line_length = getline(&line, &line_size, rewr.in)) )
	{
		if ( line_length && line[line_length - 1] == '\n' )
			line[--line_length] = '\0';
		char* dup = strdup(line);
		if ( !dup )
			return rewrite_abort(&rewr), false;
		struct fstab fsent;
		if ( !scanfsent(dup, &fsent) )
		{
			fprintf(rewr.out, "%s\n", line);
		}
		else if ( match_fstab_device(fsent.fs_spec, bdev) )
		{
			fprintf(rewr.out, "%s %s %s %s %i %i\n",
			        fsent.fs_spec,
			        mountpoint,
			        fsent.fs_vfstype,
			        fsent.fs_mntops,
			        fsent.fs_freq,
			        fsent.fs_passno);
			found = true;
		}
		else if ( !strcmp(fsent.fs_file, mountpoint) )
		{
			// Remove conflicting mountpoint.
		}
		else
		{
			fprintf(rewr.out, "%s\n", line);
		}
		free(dup);
	}
	free(line);
	if ( errno )
		return rewrite_abort(&rewr), false;
	if ( !found )
		print_blockdevice_fsent(rewr.out, bdev, mountpoint);
	return rewrite_finish(&rewr);
}

__attribute__((format(printf, 1, 2)))
static void command_error(const char* format, ...)
{
	va_list ap;
	va_start(ap, format);
	if ( !interactive )
		verr(1, format, ap);
	vfprintf(stderr, format, ap);
	fprintf(stderr, ": %s\n", strerror(errno));
	va_end(ap);
}

__attribute__((format(printf, 1, 2)))
static void command_errorx(const char* format, ...)
{
	va_list ap;
	va_start(ap, format);
	if ( !interactive )
		verrx(1, format, ap);
	vfprintf(stderr, format, ap);
	fprintf(stderr, "\n");
	va_end(ap);
}

static void unscan_partition(struct partition* p)
{
	struct blockdevice* bdev = &p->bdev;
	filesystem_release(bdev->fs);
	bdev->fs_error = FILESYSTEM_ERROR_NONE;
}

static void unscan_device()
{
	if ( !current_hd )
		return;
	for ( size_t i = 0; i < current_areas_count; i++ )
	{
		free(current_areas[i].filesystem);
		free(current_areas[i].mountpoint);
	}
	current_areas_count = 0;
	free(current_areas);
	current_areas = NULL;
	if ( current_pt )
	{
		for ( size_t i = 0; i < current_pt->partitions_count; i++ )
			unscan_partition(current_pt->partitions[i]);
		partition_table_release(current_pt);
	}
	current_pt = NULL;
	current_pt_type = PARTITION_TABLE_TYPE_UNKNOWN;
	current_hd->bdev.pt_error = PARTITION_ERROR_NONE;
}

static void scan_partition(struct partition* p)
{
	unscan_partition(p);
	struct blockdevice* bdev = &p->bdev;
	bdev->fs_error = blockdevice_inspect_filesystem(&bdev->fs, bdev);
	if ( bdev->fs_error == FILESYSTEM_ERROR_ABSENT ||
	     bdev->fs_error == FILESYSTEM_ERROR_UNRECOGNIZED )
		return;
	if ( bdev->fs_error != FILESYSTEM_ERROR_NONE )
		return command_errorx("Scanning `%s': %s", device_name(p->path),
		                      filesystem_error_string(bdev->fs_error));
}

static void scan_device()
{
	if ( !current_hd )
		return;
	unscan_device();
	struct blockdevice* bdev = &current_hd->bdev;
	if ( !blockdevice_probe_partition_table_type(&current_pt_type, bdev) )
	{
		// TODO: Try probe for a filesystem here to see if one covers the whole
		//       device.
		command_error("Scanning `%s'", device_name(current_hd->path));
		current_hd = NULL;
		current_pt_type = PARTITION_TABLE_TYPE_UNKNOWN;
		return;
	}
	bdev->pt_error = blockdevice_get_partition_table(&current_pt, bdev);
	if ( bdev->pt_error != PARTITION_ERROR_NONE )
	{
		if ( bdev->pt_error != PARTITION_ERROR_ABSENT &&
		     bdev->pt_error != PARTITION_ERROR_UNRECOGNIZED )
			command_errorx("Scanning `%s': %s", device_name(current_hd->path),
			               partition_error_string(bdev->pt_error));
		partition_table_release(current_pt);
		current_pt = NULL;
		return;
	}
	current_pt_type = current_pt->type;
	// TODO: In case of GPT, verify the header is supported for write (version
	//       check, just using it blindly is safe for read compatibility, but
	//       we need to refuse updating the GPT if uses extensions). Then after
	//       deciding this, check this condition in all commands that modify
	//       the partition table.
	for ( size_t i = 0; i < current_pt->partitions_count; i++ )
		scan_partition(current_pt->partitions[i]);
	// TODO: Check for overflow here.
	size_t areas_length = 2 * current_pt->partitions_count + 1;
	if ( current_pt_type == PARTITION_TABLE_TYPE_MBR )
	{
		struct mbr_partition_table* mbrpt =
			(struct mbr_partition_table*) current_pt->raw_partition_table;
		areas_length += mbrpt->ebr_chain_count * 2;
	}
	current_areas = (struct device_area*)
		reallocarray(NULL, sizeof(struct device_area), areas_length);
	if ( !current_areas )
	{
		command_error("malloc");
		partition_table_release(current_pt);
		current_pt = NULL;
		return;
	}
	struct device_area* sort_areas =
		current_areas + areas_length - current_pt->partitions_count;
	for ( size_t i = 0; i < current_pt->partitions_count; i++ )
	{
		struct device_area* area = &sort_areas[i];
		struct partition* p = current_pt->partitions[i];
		memset(area, 0, sizeof(*area));
		area->start = p->start;
		area->length = p->length;
		area->p = p;
		area->inside_extended = p->type == PARTITION_TYPE_LOGICAL;
	}
	qsort(sort_areas, current_pt->partitions_count, sizeof(struct device_area),
	      device_area_compare_start);
	off_t last_end = current_pt->usable_start;
	current_areas_count = 0;
	for ( size_t i = 0; i < current_pt->partitions_count; i++ )
	{
		struct device_area* area = &sort_areas[i];
		if ( area->p->type == PARTITION_TYPE_LOGICAL )
			continue;
		if ( last_end < area->start )
		{
			struct device_area* hole = &current_areas[current_areas_count++];
			memset(hole, 0, sizeof(*hole));
			hole->start = last_end;
			hole->length = area->start - last_end;
		}
		current_areas[current_areas_count++] = *area;
		last_end = area->start + area->length;
		if ( area->p->type == PARTITION_TYPE_EXTENDED &&
		     current_pt_type == PARTITION_TABLE_TYPE_MBR )
		{
			off_t extended_start = area->start;
			off_t extended_end = area->start + area->length;
			struct mbr_partition_table* mbrpt =
				(struct mbr_partition_table*) current_pt->raw_partition_table;
			assert(1 <= mbrpt->ebr_chain_count);
			size_t ebr_i = 0;
			size_t larea_i = i + 1;
			size_t new_part_ebr_i = 0;
			off_t offset = extended_start;
			while ( true )
			{
				struct mbr_ebr_link* ebr = NULL;
				if ( ebr_i < mbrpt->ebr_chain_count )
					ebr = &mbrpt->ebr_chain[ebr_i];
				struct device_area* larea = NULL;
				if ( larea_i < current_pt->partitions_count &&
				     sort_areas[larea_i].p->type == PARTITION_TYPE_LOGICAL )
					larea = &sort_areas[larea_i];
				off_t ebr_start = ebr ? ebr->offset : extended_end;
				off_t larea_start = larea ? larea->start : extended_end;
				off_t dist_ebr = ebr_start - offset;
				off_t dist_larea = larea_start - offset;
				off_t dist = dist_larea < dist_ebr ? dist_larea : dist_ebr;
				bool next_is_larea = larea && dist_larea < dist_ebr;
				bool next_is_ebr = ebr && dist_ebr < dist_larea;
				if ( next_is_ebr )
				{
					struct mbr_partition p;
					memcpy(&p, ebr->ebr.partitions[0], sizeof(p));
					mbr_partition_decode(&p);
					new_part_ebr_i = ebr_i;
					ebr_i++;
					continue;
				}
				if ( current_hd->logical_block_size < dist )
				{
					assert(ebr_i);
					struct device_area* hole = &current_areas[current_areas_count];
					memset(hole, 0, sizeof(*hole));
					hole->ebr_index = new_part_ebr_i;
					hole->ebr_off = offset;
					hole->start = offset + current_hd->logical_block_size;
					hole->length = offset + dist - hole->start;
					hole->extended_start = extended_start;
					if ( next_is_larea )
					{
						hole->length -= current_hd->logical_block_size;
						hole->ebr_move_off = hole->start + hole->length;
						assert(ebr_i);
						hole->ebr_move_index = ebr_i - 1;
					}
					hole->inside_extended = true;
					if ( 0 <= hole->length )
						current_areas_count++;
				}
				if ( next_is_larea )
				{
					current_areas[current_areas_count++] = *larea;
					offset = larea->start + larea->length;
					larea_i++;
					new_part_ebr_i++;
				}
				else
					break;
			}
			i = larea_i - 1;
		}
	}
	if ( last_end < current_pt->usable_end )
	{
		struct device_area* hole = &current_areas[current_areas_count++];
		memset(hole, 0, sizeof(*hole));
		hole->start = last_end;
		hole->length = current_pt->usable_end - last_end;
	}
	size_t new_areas_count = 0;
	for ( size_t i = 0; i < current_areas_count; i++ )
	{
		if ( !current_areas[i].p )
		{
			uintmax_t mask = ~(UINTMAX_C(1048576) - 1);
			off_t start = current_areas[i].start;
			uintmax_t aligned = (uintmax_t) start;
			aligned = -(-aligned & mask);
			off_t start_aligned = (off_t) aligned;
			if ( current_areas[i].length < start_aligned - start )
				continue;
			current_areas[i].start = start_aligned;
			current_areas[i].length -= start_aligned - start;
			current_areas[i].length &= mask;
			if ( current_areas[i].length == 0 )
				continue;
		}
		if ( new_areas_count != i )
			current_areas[new_areas_count] = current_areas[i];
		new_areas_count++;
	}
	current_areas_count = new_areas_count;
}

static void switch_device(struct harddisk* hd)
{
	if ( current_hd )
	{
		unscan_device();
		current_hd = NULL;
	}
	if ( !(current_hd = hd) )
		return;
	scan_device();
}

static bool lookup_partition_by_string(struct partition** out,
                                       const char* argv0,
                                       const char* numstr)
{
	char* end;
	unsigned long part_index = strtoul(numstr, &end, 10);
	if ( *end )
	{
		command_errorx("%s: Invalid partition number `%s'", argv0, numstr);
		return false;
	}
	struct partition* part = NULL;
	for ( size_t i = 0; i < current_pt->partitions_count; i++ )
	{
		if ( current_pt->partitions[i]->index == part_index )
		{
			part = current_pt->partitions[i];
			break;
		}
	}
	if ( !part)
	{
		command_errorx("%s: No such partition `%sp%lu'", argv0,
		               device_name(current_hd->path), part_index);
		return false;
	}
	return *out = part, true;
}

bool gpt_update(const char* argv0, struct gpt_partition_table* gptpt)
{
	size_t header_size;
	blksize_t logical_block_size = current_hd->logical_block_size;
	struct gpt pri_gpt;
	memcpy(&pri_gpt, &gptpt->gpt, sizeof(pri_gpt));
	gpt_decode(&pri_gpt);
	size_t rpt_size = (size_t) pri_gpt.number_of_partition_entries *
	                  (size_t) pri_gpt.size_of_partition_entry;
	uint32_t rpt_checksum = gpt_crc32(gptpt->rpt, rpt_size);
	uint64_t pri_gpt_lba = 1;
	off_t pri_gpt_off = (off_t) logical_block_size * (off_t) pri_gpt_lba;
	uint64_t alt_gpt_lba = pri_gpt.alternate_lba;
	off_t alt_gpt_off = (off_t) logical_block_size * (off_t) alt_gpt_lba;
	struct gpt alt_gpt;
	if ( preadall(current_hd->fd, &alt_gpt, sizeof(alt_gpt),
	              alt_gpt_off) < sizeof(alt_gpt) )
	{
		command_error("%s: %s: read", argv0, device_name(current_hd->path));
		return false;
	}
	gpt_decode(&alt_gpt);
	// TODO: Validate the alternate gpt.
	uint64_t alt_rpt_lba = alt_gpt.partition_entry_lba;
	off_t alt_rpt_off = (off_t) logical_block_size * (off_t) alt_rpt_lba;
	if ( pwriteall(current_hd->fd, gptpt->rpt, rpt_size,
	               alt_rpt_off) < rpt_size )
	{
		command_error("%s: %s: write", argv0, device_name(current_hd->path));
		scan_device();
		return false;
	}
	memcpy(&alt_gpt, &gptpt->gpt, sizeof(alt_gpt));
	gpt_decode(&alt_gpt);
	alt_gpt.header_crc32 = 0;
	alt_gpt.my_lba = alt_gpt_lba;
	alt_gpt.alternate_lba = pri_gpt_lba;
	alt_gpt.partition_entry_lba = alt_rpt_lba;
	alt_gpt.partition_entry_array_crc32 = rpt_checksum;
	header_size = alt_gpt.header_size;
	gpt_encode(&alt_gpt);
	alt_gpt.header_crc32 = htole32(gpt_crc32(&alt_gpt, header_size));
	if ( pwriteall(current_hd->fd, &alt_gpt, sizeof(alt_gpt),
	               alt_gpt_off) < sizeof(alt_gpt) )
	{
		command_error("%s: %s: write", argv0, device_name(current_hd->path));
		scan_device();
		return false;
	}
	if ( fsync(current_hd->fd) < 0 )
	{
		command_error("%s: %s: sync", argv0, device_name(current_hd->path));
		scan_device();
		return false;
	}
	uint64_t pri_rpt_lba = pri_gpt.partition_entry_lba;
	off_t pri_rpt_off = (off_t) logical_block_size * (off_t) pri_rpt_lba;
	if ( pwriteall(current_hd->fd, gptpt->rpt, rpt_size,
	               pri_rpt_off) < rpt_size )
	{
		command_error("%s: %s: write", argv0, device_name(current_hd->path));
		scan_device();
		return false;
	}
	memcpy(&pri_gpt, &gptpt->gpt, sizeof(pri_gpt));
	gpt_decode(&pri_gpt);
	pri_gpt.header_crc32 = 0;
	pri_gpt.my_lba = pri_gpt_lba;
	pri_gpt.alternate_lba = alt_gpt_lba;
	pri_gpt.partition_entry_lba = pri_rpt_lba;
	pri_gpt.partition_entry_array_crc32 = rpt_checksum;
	header_size = pri_gpt.header_size;
	gpt_encode(&pri_gpt);
	pri_gpt.header_crc32 = htole32(gpt_crc32(&pri_gpt, header_size));
	if ( pwriteall(current_hd->fd, &pri_gpt, sizeof(pri_gpt),
	               pri_gpt_off) < sizeof(pri_gpt) )
	{
		command_error("%s: %s: write", argv0, device_name(current_hd->path));
		scan_device();
		return false;
	}
	if ( fsync(current_hd->fd) < 0 )
	{
		command_error("%s: %s: sync", argv0, device_name(current_hd->path));
		scan_device();
		return false;
	}
	return true;
}

bool create_partition_device(const char* argv0, const struct partition* p)
{
	int mountfd = open(p->path, O_RDONLY | O_CREAT | O_EXCL);
	if ( mountfd < 0 )
	{
		command_error("%s: %s", argv0, p->path);
		return false;
	}
	int partfd = mkpartition(current_hd->fd, p->start, p->length);
	if ( partfd < 0 )
	{
		close(mountfd);
		command_error("%s: mkpartition: %s", argv0, p->path);
		return false;
	}
	if ( fsm_fsbind(partfd, mountfd, 0) < 0 )
	{
		command_error("%s: fsbind: %s", argv0, p->path);
		return false;
	}
	close(partfd);
	close(mountfd);
	return true;
}

struct command
{
	const char* name;
	void (*function)(size_t argc, char** argv);
	const char* flags;
};

static const struct command commands[];
static const size_t commands_count;

static void on_device(size_t argc, char** argv)
{
	if ( argc < 2 )
	{
		printf("%s\n", current_hd ? device_name(current_hd->path) : "none");
		return;
	}
	const char* name = argv[1];
	if ( 2 < argc )
	{
		command_errorx("%s: extra operand `%s'", argv[0], argv[2]);
		return;
	}
	if ( !strcmp(name, "none") )
	{
		current_hd = NULL;
		return;
	}
	for ( size_t i = 0; i < hds_count; i++ )
	{
		char buf[sizeof(i) * 3];
		snprintf(buf, sizeof(buf), "%zu", i);
		if ( strcmp(name, hds[i]->path) != 0 &&
		     strcmp(name, device_name(hds[i]->path)) != 0 &&
		     strcmp(name, buf) != 0 )
			continue;
		switch_device(hds[i]);
		return;
	}
	command_errorx("%s: No such device `%s'", argv[0], name);
}

static char* display_harddisk_format(void* ctx, size_t row, size_t column)
{
	if ( row == 0 )
	{
		switch ( column )
		{
		case 0: return strdup("#");
		case 1: return strdup("DEVICE");
		case 2: return strdup("SIZE");
		case 3: return strdup("MODEL");
		case 4: return strdup("SERIAL");
		default: return NULL;
		}
	}
	struct harddisk** hds = (struct harddisk**) ctx;
	struct harddisk* hd = hds[row-1];
	switch ( column )
	{
	case 0: return print_string("%zu", row - 1);
	case 1: return strdup(device_name(hd->path));
	case 2: return format_bytes_amount((uintmax_t) hd->st.st_size);
	case 3: return strdup(hd->model);
	case 4: return strdup(hd->serial);
	default: return NULL;
	}
}

static void on_devices(size_t argc, char** argv)
{
	(void) argc;
	(void) argv;
	qsort(hds, hds_count, sizeof(struct harddisk*), harddisk_compare_path);
	display_rows_columns(display_harddisk_format, hds, 1 + hds_count, 5);
}

static void on_fsck(size_t argc, char** argv)
{
	if ( argc < 2 )
	{
		command_errorx("%s: No partition specified", argv[0]);
		return;
	}
	struct partition* p;
	if ( !lookup_partition_by_string(&p, argv[0], argv[1]) )
		return;
	if ( !p->bdev.fs )
	{
		command_errorx("%s: %s: No filesystem recognized", argv[0],
		               device_name(p->path));
		return;
	}
	struct filesystem* fs = p->bdev.fs;
	if ( !fs->fsck )
	{
		command_errorx("%s: %s: fsck is not supported for %s", argv[0],
		               device_name(p->path), fs->fstype_name);
		return;
	}
	bool interactive_fsck = false;
	// TODO: Run this in its own foreground process group so it can be ^C'd.
	pid_t child_pid;
retry_interactive_fsck:
	if ( (child_pid = fork()) < 0 )
	{
		command_error("%s: fork", argv[0]);
		return;
	}
	if ( child_pid == 0 )
	{
		if ( interactive_fsck )
			execlp(fs->fsck, fs->fsck, "--", p->path, (const char*) NULL);
		else
			execlp(fs->fsck, fs->fsck, "-p", "--", p->path, (const char*) NULL);
		warn("%s: Failed to load filesystem checker: %s", argv[0], fs->fsck);
		_Exit(127);
	}
	int code;
	waitpid(child_pid, &code, 0);
	if ( WIFSIGNALED(code) )
		command_errorx("%s: %s: Filesystem check failed: %s: %s", argv[0],
		               p->path, fs->fsck, strsignal(WTERMSIG(code)));
	else if ( !WIFEXITED(code) )
		command_errorx("%s: %s: Filesystem check failed: %s: %s", argv[0],
		               p->path, fs->fsck, "Unexpected unusual termination");
	else if ( WEXITSTATUS(code) == 127 )
		command_errorx("%s: %s: Filesystem check failed: %s: %s", argv[0],
		               p->path, fs->fsck, "Filesystem checker is not installed");
	else if ( WEXITSTATUS(code) & 4 && !interactive_fsck )
	{
		interactive_fsck = true;
		goto retry_interactive_fsck;
	}
	else if ( WEXITSTATUS(code) != 0 && WEXITSTATUS(code) != 1 )
		command_errorx("%s: %s: Filesystem check failed: %s: %s", argv[0],
		               p->path, fs->fsck, "Filesystem checker was unsuccessful");
}

static void on_help(size_t argc, char** argv)
{
	(void) argc;
	(void) argv;
	// TODO: Show help for a particular command if an argument is given.
	//       Perhaps advertise the man page?
	const char* prefix = "";
	for ( size_t i = 0; i < commands_count; i++ )
	{
		if ( strchr(commands[i].flags, 'a') )
			continue;
		printf("%s%s", prefix, commands[i].name);
		prefix = " ";
	}
	printf("\n");
}

static char* display_area_format(void* ctx, size_t row, size_t column)
{
	if ( row == 0 )
	{
		switch ( column )
		{
		case 0: return strdup("PARTITION");
		case 1: return strdup("SIZE");
		case 2: return strdup("FILESYSTEM");
		case 3: return strdup("MOUNTPOINT");
		// TODO: LABEL
		default: return NULL;
		}
	}
	struct device_area* areas = (struct device_area*) ctx;
	struct device_area* area = &areas[row - 1];
	if ( !area->p )
	{
		switch ( column )
		{
		case 0: return strdup(area->inside_extended ? "  (unused)" : "(unused)");
		case 1: return format_bytes_amount((uintmax_t) area->length);
		case 2: return strdup("-");
		case 3: return strdup("-");
		default: return NULL;
		}
	}
	switch ( column )
	{
	case 0:
		if ( area->inside_extended )
			return print_string("  %s", device_name(area->p->path));
		else
			return strdup(device_name(area->p->path));
	case 1: return format_bytes_amount((uintmax_t) area->length);
	case 2:
		if ( area->p->bdev.fs )
			return strdup(area->p->bdev.fs->fstype_name);
		switch ( area->p->bdev.fs_error )
		{
		case FILESYSTEM_ERROR_NONE: return strdup("(no error)");
		case FILESYSTEM_ERROR_ABSENT: return strdup("none");
		case FILESYSTEM_ERROR_UNRECOGNIZED: return strdup("unrecognized");
		case FILESYSTEM_ERROR_ERRNO: return strdup("(error)");
		}
		return strdup("(unknown error)");
	case 3:
	{
		struct blockdevice* bdev = &area->p->bdev;
		char* storage;
		struct fstab fsent;
		if ( lookup_fstab_by_blockdevice(&fsent, &storage, bdev) )
		{
			char* result = strdup(fsent.fs_file);
			free(storage);
			return result;
		}
		return strdup("-");
	}
	default: return NULL;
	}
}

static void on_ls(size_t argc, char** argv)
{
	(void) argc;
	(void) argv;
	display_rows_columns(display_area_format, current_areas,
	                     1 + current_areas_count, 4);
}

static void on_man(size_t argc, char** argv)
{
	(void) argc;
	sigset_t oldset, sigttou;
	sigemptyset(&sigttou);
	sigaddset(&sigttou, SIGTTOU);
	pid_t child_pid = fork();
	if ( child_pid < 0 )
	{
		command_error("%s: fork", argv[0]);
		return;
	}
	if ( child_pid == 0 )
	{
		setpgid(0, 0);
		sigprocmask(SIG_BLOCK, &sigttou, &oldset);
		tcsetpgrp(0, getpgid(0));
		sigprocmask(SIG_SETMASK, &oldset, NULL);
		const char* defargv[] = { "man", "8", "disked", NULL };
		char** subargv = argc == 1 ? (char**) defargv : argv;
		execvp(subargv[0], (char* const*) subargv);
		warn("%s", subargv[0]);
		_exit(127);
	}
	int code;
	waitpid(child_pid, &code, 0);
	sigprocmask(SIG_BLOCK, &sigttou, &oldset);
	tcsetpgrp(0, getpgid(0));
	sigprocmask(SIG_SETMASK, &oldset, NULL);
}

static char* display_hole_format(void* ctx, size_t row, size_t column)
{
	if ( row == 0 )
	{
		switch ( column )
		{
		case 0: return strdup("HOLE");
		case 1: return strdup("START");
		case 2: return strdup("LENGTH");
		case 3: return strdup("TYPE");
		default: return NULL;
		}
	}
	struct device_area* areas = (struct device_area*) ctx;
	struct device_area* hole = NULL;
	size_t num_hole = 0;
	for ( size_t i = 0; i < current_areas_count; i++ )
	{
		if ( areas[i].p )
			continue;
		if ( num_hole == row - 1 )
		{
			hole = &areas[i];
			break;
		}
		num_hole++;
	}
	switch ( column )
	{
	case 0: return print_string("%zu", row);
	case 1: return format_bytes_amount((uintmax_t) hole->start);
	case 2: return format_bytes_amount((uintmax_t) hole->length);
	case 3: return strdup(hole->inside_extended ? "logical" : "primary");
	default: return NULL;
	}
}

static void on_mkpart(size_t argc, char** argv)
{
	(void) argc;
	(void) argv;
	size_t num_holes = 0;
	struct device_area* hole = NULL;
	for ( size_t i = 0; i < current_areas_count; i++ )
	{
		if ( current_areas[i].p )
			continue;
		num_holes++;
		hole = &current_areas[i];
	}
	if ( num_holes == 0 )
	{
		command_errorx("%s: %s: Device has no unused areas left",
		               argv[0], device_name(current_hd->path));
		return;
	}
	else if ( 2 <= num_holes )
	{
		bool type_column = current_pt_type == PARTITION_TABLE_TYPE_MBR;
		display_rows_columns(display_hole_format, current_areas,
		                     1 + num_holes, type_column ? 4 : 3);
		char answer[sizeof(size_t) * 3];
		while ( true )
		{
			prompt(answer, sizeof(answer),
			       "Which hole to create the partition inside?", "1");
			char* end;
			unsigned long num = strtoul(answer, &end, 10);
			if ( *end || num_holes < num )
			{
				command_errorx("%s: Invalid hole `%s'", argv[0], answer);
				continue;
			}
			for ( size_t i = 0; i < current_areas_count; i++ )
			{
				if ( current_areas[i].p )
					continue;
				if ( --num != 0 )
					continue;
				hole = &current_areas[i];
				break;
			}
			break;
		}
		printf("\n");
	}
	unsigned int slot = 0;
	if ( current_pt_type == PARTITION_TABLE_TYPE_MBR && hole->inside_extended )
	{
		slot = 5 + hole->ebr_index;
	}
	else if ( current_pt_type == PARTITION_TABLE_TYPE_MBR )
	{
		struct mbr_partition_table* mbrpt =
			(struct mbr_partition_table*) current_pt->raw_partition_table;
		for ( unsigned int i = 0; i < 4; i++ )
		{
			struct mbr_partition p;
			memcpy(&p, &mbrpt->mbr.partitions[i], sizeof(p));
			mbr_partition_decode(&p);
			if ( mbr_is_partition_used(&p) )
				continue;
			slot = 1 + i;
			break;
		}
	}
	else if ( current_pt_type == PARTITION_TABLE_TYPE_GPT )
	{
		struct gpt_partition_table* gptpt =
			(struct gpt_partition_table*) current_pt->raw_partition_table;
		struct gpt gpt;
		memcpy(&gpt, &gptpt->gpt, sizeof(gpt));
		gpt_decode(&gpt);
		for ( uint32_t i = 0; i < gpt.number_of_partition_entries; i++ )
		{
			size_t poff = i * (size_t) gpt.size_of_partition_entry;
			struct gpt_partition p;
			memcpy(&p, gptpt->rpt + poff, sizeof(p));
			bool unused = true;
			for ( size_t n = 0; n < 16; n++ )
				if ( p.partition_type_guid[n] )
					unused = false;
			if ( !unused )
				continue;
			slot = 1 + i;
			break;
		}
	}
	else
	{
		command_errorx("%s: %s: Partition scheme not supported", argv[0],
		               device_name(current_hd->path));
		return;
	}
	if ( slot == 0 )
	{
		command_errorx("%s: %s: Cannot add partition because the table is full",
		               argv[0], device_name(current_hd->path));
		return;
	}
	char* start_str = format_bytes_amount((uintmax_t) hole->start);
	assert(start_str); // TODO: Error handling.
	char* length_str = format_bytes_amount((uintmax_t) hole->length);
	assert(length_str); // TODO: Error handling.
	printf("Creating partition inside hole at %s of length %s (100%%)\n",
	       start_str, length_str);
	free(start_str);
	free(length_str);
	off_t start;
	while ( true )
	{
		char answer[256];
		prompt(answer, sizeof(answer),
		       "Free space before partition? (42%/15G/...)", "0%");
		if ( !parse_disk_quantity(&start, answer, hole->length) )
		{
			fprintf(stderr, "Invalid quantity `%s'.\n", answer);
			continue;
		}
		if ( start == hole->length )
		{
			fprintf(stderr, "Answer was all free space, but need space for the "
			                "partition itself.\n");
			continue;
		}
		break;
	}
	printf("\n");
	off_t max_length = hole->length - start;
	off_t length;
	length_str = format_bytes_amount((uintmax_t) max_length);
	assert(length_str);
	printf("Partition size can be at most %s (100%%).\n", length_str);
	free(length_str);
	while ( true )
	{
		char answer[256];
		prompt(answer, sizeof(answer),
		       "Partition size? (42%/15G/...)", "100%");
		if ( !parse_disk_quantity(&length, answer, max_length) )
		{
			fprintf(stderr, "Invalid quantity `%s'.\n", answer);
			continue;
		}
		if ( length == 0 )
		{
			fprintf(stderr, "Answer was zero (or rounded down to zero).\n");
			continue;
		}
		break;
	}
	printf("\n");
	char fstype[256];
	while ( true )
	{
		bool is_mbr = current_pt_type == PARTITION_TABLE_TYPE_MBR;
		bool is_gpt = current_pt_type == PARTITION_TABLE_TYPE_GPT;
		const char* question = "Format a filesystem? (no/ext2)";
		if ( is_mbr )
			question = "Format a filesystem? (no/ext2/extended)";
		else if ( is_gpt )
			question = "Format a filesystem? (no/ext2/biosboot)";
		prompt(fstype, sizeof(fstype), question, "ext2");
		if ( strcmp(fstype, "no") != 0 &&
		     strcmp(fstype, "ext2") != 0 &&
		     (!is_mbr || strcmp(fstype, "extended") != 0) &&
		     (!is_gpt || strcmp(fstype, "biosboot") != 0) )
		{
			fprintf(stderr, "Invalid filesystem choice `%s'.\n", fstype);
			continue;
		}
		if ( !strcmp(fstype, "extended") )
		{
			for ( size_t i = 0; i < current_pt->partitions_count; i++ )
			{
				if ( current_pt->partitions[i]->type != PARTITION_TYPE_EXTENDED )
					continue;
				command_errorx("%s: %s: Device already has an extended partition",
		                       argv[0], device_name(current_hd->path));
				return;
			}
		}
		break;
	}
	char mountpoint[256] = "";
	bool mountable = !strcmp(fstype, "ext2");
	while ( mountable )
	{
		prompt(mountpoint, sizeof(mountpoint),
		       "Where to mount partition? (mountpoint or 'no')", "no");
		if ( !strcmp(mountpoint, "no") )
		{
			mountpoint[0] = '\0';
			break;
		}
		if ( !strcmp(mountpoint, "mountpoint") )
		{
			printf("Then answer which mountpoint.\n");
			continue;
		}
		if ( !verify_mountpoint(mountpoint) )
		{
			fprintf(stderr, "Invalid mountpoint `%s'.\n", fstype);
			continue;
		}
		simplify_mountpoint(mountpoint);
		break;
	}
	printf("\n");
	size_t renumbered_partitions = 0;
	if ( current_pt_type == PARTITION_TABLE_TYPE_MBR && hole->inside_extended )
	{
		struct mbr_partition_table* mbrpt =
			(struct mbr_partition_table*) current_pt->raw_partition_table;
		struct mbr ebr;
		struct mbr_partition p;
		off_t next_ebr_off = 0;
		uint32_t next_ebr_full_sectors = 0;
		if ( hole->ebr_index + 1 < mbrpt->ebr_chain_count )
		{
			memcpy(&ebr, &mbrpt->ebr_chain[hole->ebr_index + 1].ebr, sizeof(ebr));
			memcpy(&p, &ebr.partitions[0], sizeof(p));
			mbr_partition_decode(&p);
			next_ebr_full_sectors = p.start_sector + p.total_sectors;
			next_ebr_off = mbrpt->ebr_chain[hole->ebr_index + 1].offset;
		}
		if ( hole->ebr_move_off )
		{
			renumbered_partitions = 5 + hole->ebr_move_index;
			memcpy(&ebr, &mbrpt->ebr_chain[hole->ebr_move_index].ebr, sizeof(ebr));
			memcpy(&p, &ebr.partitions[0], sizeof(p));
			mbr_partition_decode(&p);
			// TODO: Update CHS information?
			p.start_sector = 1;
			next_ebr_full_sectors = p.start_sector + p.total_sectors;
			mbr_partition_encode(&p);
			memcpy(&ebr.partitions[0], &p, sizeof(p));
			memcpy(&p, &ebr.partitions[1], sizeof(p));
			mbr_partition_decode(&p);
			// TODO: Update CHS information?
			p.start_sector = 0;
			if ( next_ebr_off )
			{
				assert(hole->ebr_move_off < next_ebr_off);
				off_t dist = next_ebr_off - hole->extended_start;
				assert(dist);
				p.start_sector = dist / current_hd->logical_block_size;
			}
			mbr_partition_encode(&p);
			memcpy(&ebr.partitions[1], &p, sizeof(p));
			if ( pwriteall(current_hd->fd, &ebr, sizeof(ebr),
				           hole->ebr_move_off) < sizeof(ebr) )
			{
				command_error("%s: %s: write", argv[0], device_name(current_hd->path));
				scan_device();
				return;
			}
			next_ebr_off = hole->ebr_move_off;
		}
		off_t ebr_off = hole->ebr_off;
		memset(&ebr, 0, sizeof(ebr));
		ebr.signature[0] = 0x55;
		ebr.signature[1] = 0xAA;
		memset(&p, 0, sizeof(p));
		off_t p_start = hole->start + start - ebr_off;
		p.flags = 0;
		p.start_head = 1; // TODO: This.
		p.start_sector_cylinder = 1; // TODO: This.
		if ( !strcmp(fstype, "ext2") )
			p.system_id = 0x83;
		else if ( !strcmp(fstype, "extended") )
			p.system_id = 0x05;
		else
			p.system_id = 0x83;
		p.end_head = 2; // TODO: This.
		p.end_sector_cylinder = 2; // TODO: This.
		p.start_sector = p_start / current_hd->logical_block_size;
		p.total_sectors = length / current_hd->logical_block_size;
		mbr_partition_encode(&p);
		memcpy(&ebr.partitions[0], &p, sizeof(p));
		memset(&p, 0, sizeof(p));
		p.system_id = 0x00;
		p.start_sector = 0;
		p.total_sectors = 0;
		if ( next_ebr_off )
		{
			p.system_id = 0x05;
			assert(ebr_off < next_ebr_off);
			off_t dist = next_ebr_off - hole->extended_start;
			assert(dist);
			p.start_sector = dist / current_hd->logical_block_size;
			p.total_sectors = next_ebr_full_sectors;
		}
		mbr_partition_encode(&p);
		memcpy(&ebr.partitions[1], &p, sizeof(p));
		if ( pwriteall(current_hd->fd, &ebr, sizeof(ebr),
		               ebr_off) < sizeof(ebr) )
		{
			command_error("%s: %s: write", argv[0], device_name(current_hd->path));
			scan_device();
			return;
		}
		if ( 0 < hole->ebr_index )
		{
			size_t prev_ebr_index = hole->ebr_index - 1;
			off_t prev_ebr_off = mbrpt->ebr_chain[prev_ebr_index].offset;
			memcpy(&ebr, &mbrpt->ebr_chain[prev_ebr_index].ebr, sizeof(ebr));
			memcpy(&p, &ebr.partitions[1], sizeof(p));
			mbr_partition_decode(&p);
			// TODO: Update CHS information?
			p.system_id = 0x05;
			assert(prev_ebr_off < ebr_off);
			off_t dist = ebr_off - hole->extended_start;
			assert(dist);
			p.start_sector = dist / current_hd->logical_block_size;
			off_t dist_total = hole->start + start + length - ebr_off;
			assert(dist_total);
			p.total_sectors = dist_total / current_hd->logical_block_size;
			mbr_partition_encode(&p);
			memcpy(&ebr.partitions[1], &p, sizeof(p));
			if ( pwriteall(current_hd->fd, &ebr, sizeof(ebr),
				           prev_ebr_off) < sizeof(ebr) )
			{
				command_error("%s: %s: write", argv[0], device_name(current_hd->path));
				scan_device();
				return;
			}
		}
		if ( fsync(current_hd->fd) < 0 )
		{
			command_error("%s: %s: sync", argv[0], device_name(current_hd->path));
			scan_device();
			return;
		}
	}
	else if ( current_pt_type == PARTITION_TABLE_TYPE_MBR )
	{
		struct mbr_partition_table* mbrpt =
			(struct mbr_partition_table*) current_pt->raw_partition_table;
		struct mbr mbr;
		memcpy(&mbr, &mbrpt->mbr, sizeof(mbr));
		struct mbr_partition p;
		memset(&p, 0, sizeof(p));
		p.flags = 0;
		p.start_head = 0; // TODO: This.
		p.start_sector_cylinder = 0; // TODO: This.
		if ( !strcmp(fstype, "ext2") )
			p.system_id = 0x83;
		else if ( !strcmp(fstype, "extended") )
			p.system_id = 0x05;
		else
			p.system_id = 0x83;
		p.end_head = 0; // TODO: This.
		p.end_sector_cylinder = 0; // TODO: This.
		p.start_sector = (hole->start + start) / current_hd->logical_block_size;
		p.total_sectors = length / current_hd->logical_block_size;
		mbr_partition_encode(&p);
		memcpy(&mbr.partitions[slot - 1], &p, sizeof(p));
		if ( pwriteall(current_hd->fd, &mbr, sizeof(mbr), 0) < sizeof(mbr) )
		{
			command_error("%s: %s: write", argv[0], device_name(current_hd->path));
			scan_device();
			return;
		}
		if ( fsync(current_hd->fd) < 0 )
		{
			command_error("%s: %s: sync", argv[0], device_name(current_hd->path));
			scan_device();
			return;
		}
	}
	else if ( current_pt_type == PARTITION_TABLE_TYPE_GPT )
	{
		struct gpt_partition_table* gptpt =
			(struct gpt_partition_table*) current_pt->raw_partition_table;
		struct gpt gpt;
		memcpy(&gpt, &gptpt->gpt, sizeof(gpt));
		gpt_decode(&gpt);
		size_t poff = (slot - 1) * (size_t) gpt.size_of_partition_entry;
		struct gpt_partition p;
		memset(&p, 0, sizeof(p));
		// TODO: This string might need to have some bytes swapped.
		// TODO: Perhaps just to hell with Linux guids and allocate our own
		//       Sortix values that denote particular filesystems.
		const char* type_uuid_str = "0FC63DAF-8483-4772-8E79-3D69D8477DE4";
		if ( !strcmp(fstype, "biosboot") )
			type_uuid_str = BIOSBOOT_GPT_TYPE_UUID;
		uuid_from_string(p.partition_type_guid, type_uuid_str);
		arc4random_buf(p.unique_partition_guid, sizeof(p.unique_partition_guid));
		off_t pstart = hole->start + start;
		off_t pend = hole->start + start + length;
		p.starting_lba = pstart / current_hd->logical_block_size;
		p.ending_lba = pend / current_hd->logical_block_size - 1;
		p.attributes = 0;
		// TODO: Partition name.
		gpt_partition_encode(&p);
		memcpy(gptpt->rpt + poff, &p, sizeof(p));
		if ( !gpt_update(argv[0], gptpt) )
			return;
	}
	else
	{
		command_errorx("%s: %s: Partition scheme not supported", argv[0],
		               device_name(current_hd->path));
		return;
	}
	off_t search_target_offset = hole->start + start;
	if ( current_pt_type == PARTITION_TABLE_TYPE_MBR &&
	     !strcmp(fstype, "extended") )
	{
		struct mbr ebr;
		memset(&ebr, 0, sizeof(ebr));
		ebr.signature[0] = 0x55;
		ebr.signature[1] = 0xAA;
		if ( pwriteall(current_hd->fd, &ebr, sizeof(ebr),
		               search_target_offset) < sizeof(ebr) )
		{
			command_error("%s: %s: write", argv[0], device_name(current_hd->path));
			scan_device();
			return;
		}
	}
	if ( renumbered_partitions )
	{
		for ( size_t i = 0; i < current_pt->partitions_count; i++ )
		{
			if ( current_pt->partitions[i]->index < renumbered_partitions )
				continue;
			remove_partition_device(current_pt->partitions[i]->path);
			break;
		}
	}
	scan_device();
	if ( !current_pt ) // TODO: Assumes scan went well.
	{
		command_errorx("%s: %s: Rescan failed",
		               argv[0], device_name(current_hd->path));
		return;
	}
	if ( renumbered_partitions )
	{
		for ( size_t i = 0; i < current_pt->partitions_count; i++ )
		{
			if ( current_pt->partitions[i]->index < renumbered_partitions )
				continue;
			if ( current_pt->partitions[i]->start == search_target_offset )
				continue;
			struct partition* p = current_pt->partitions[i];
			if ( !create_partition_device(argv[0], p) )
				return;
		}
	}
	struct partition* p = NULL;
	for ( size_t i = 0; i < current_pt->partitions_count; i++ )
	{
		if ( current_pt->partitions[i]->start != search_target_offset )
			continue;
		p = current_pt->partitions[i];
	}
	if ( !p )
	{
		command_errorx("%s: %s: Failed to locate expected %sp%u partition",
		               argv[0], device_name(current_hd->path),
		               device_name(current_hd->path), slot);
		return; // TODO: Something went wrong.
	}
	if ( !create_partition_device(argv[0], p) )
		return;
	printf("(Made %s)\n", device_name(p->path));
	if ( !strcmp(fstype, "ext2") )
	{
		printf("(Formatting %s as ext2...)\n", device_name(p->path));
		struct ext2_superblock zero_sb;
		memset(&zero_sb, 0, sizeof(zero_sb));
		// TODO: Add a blockdevice_pwriteall to libmount and use it.
		if ( pwriteall(current_hd->fd, &zero_sb, sizeof(zero_sb),
		               p->start + 1024) < sizeof(zero_sb) )
		{
			command_error("%s: %s: write", argv[0], device_name(current_hd->path));
			scan_partition(p);
			return;
		}
		if ( fsync(current_hd->fd) < 0 )
		{
			command_error("%s: %s: sync", argv[0], device_name(current_hd->path));
			scan_partition(p);
			return;
		}
		// TODO: Run this in its own foreground process group so ^C works.
		pid_t child_pid = fork();
		if ( child_pid < 0 )
		{
			command_error("%s: fork", argv[0]);
			return;
		}
		const char* mkfs_argv[] =
		{
			"mkfs.ext2",
			"-q",
			mountpoint[0] ? "-M" : p->path,
			mountpoint[0] ? mountpoint : NULL,
			p->path,
			NULL
		};
		if ( child_pid == 0 )
		{
			execvp(mkfs_argv[0], (char* const*) mkfs_argv);
			warn("%s", mkfs_argv[0]);
			_exit(127);
		}
		int status;
		waitpid(child_pid, &status, 0);
		if ( WIFEXITED(status) && WEXITSTATUS(status) == 127 )
		{
			command_errorx("%s: Failed to format filesystem (%s is not installed)",
			               argv[0], mkfs_argv[0]);
			return;
		}
		else if ( WIFEXITED(status) && WEXITSTATUS(status) != 0 )
		{
			command_errorx("%s: Failed to format filesystem", argv[0]);
			return;
		}
		else if ( WIFSIGNALED(status) )
		{
			command_errorx("%s: Failed to format filesystem (%s)",
			               argv[0], strsignal(WTERMSIG(status)));
			return;
		}
		printf("(Formatted %s as ext2)\n", device_name(p->path));
		scan_partition(p);
		if ( !p->bdev.fs || !(p->bdev.fs->flags & FILESYSTEM_FLAG_UUID) )
		{
			command_errorx("%s: %s: Failed to scan expected ext2 filesystem",
			               argv[0], device_name(p->path));
			return;
		}
		if ( mountpoint[0] )
		{
			if ( !add_blockdevice_to_fstab(&p->bdev, mountpoint) )
			{
				command_error("%s: %s: Failed to add partition", argv[0], fstab_path);
				return;
			}
		}
	}
}

static void on_mktable(size_t argc, char** argv)
{
	(void) argc;
	(void) argv;
	if ( current_pt_type != PARTITION_TABLE_TYPE_NONE )
	{
		const char* name = device_name(current_hd->path);
		if ( interactive )
			fprintf(stderr, "Device `%s' already has a partition table.\n", name);
		else
			command_errorx("Device `%s' already has a partition table", name);
		return;
	}
	char type_answer[32];
	const char* type = NULL;
	if ( 2 <= argc )
		type = argv[1];
	if ( !type )
	{
		prompt(type_answer, sizeof(type_answer),
		       "Which partition table type? (mbr/gpt)", "gpt");
		type = type_answer;
	}
	remove_partition_devices(current_hd->path);
	int fd = current_hd->fd;
	const char* name = device_name(current_hd->path);
	size_t logical_block_size = current_hd->logical_block_size;
	if ( !strcasecmp(type, "mbr") )
	{
		struct mbr mbr;
		memset(&mbr, 0, sizeof(mbr));
		mbr.signature[0] = 0x55;
		mbr.signature[1] = 0xAA;
		if ( pwriteall(fd, &mbr, sizeof(mbr), 0) < sizeof(mbr) )
		{
			command_error("%s: %s: write", argv[0], name);
			scan_device();
			return;
		}
	}
	else if ( !strcasecmp(type, "gpt") )
	{
		size_t header_size;
		uint64_t sector_count = current_hd->st.st_size / logical_block_size;
		size_t partition_table_size = 16384;
		if ( partition_table_size < logical_block_size )
			partition_table_size = logical_block_size;
		size_t partition_table_length =
			partition_table_size / sizeof(struct gpt_partition);
		uint64_t partition_table_sectors =
			partition_table_size / logical_block_size;
		uint64_t minimum_leading = 1 + 1 + partition_table_sectors;
		uint64_t minimum_trailing = partition_table_sectors + 1;
		uint64_t minimum_sector_count = minimum_leading + minimum_trailing;
		if ( sector_count <= minimum_sector_count )
		{
			command_errorx("Device `%s' is too small for GPT", name);
			return;
		}
		uint64_t last_lba = sector_count - 1;
		off_t gpt_off = logical_block_size;
		off_t alt_off = logical_block_size * last_lba;
		unsigned char* partition_table =
			(unsigned char*) calloc(1, partition_table_size);
		if ( !partition_table )
		{
			command_error("%s: %s: malloc", argv[0], name);
			return;
		}
		uint32_t partition_table_checksum =
			gpt_crc32(partition_table, partition_table_size);
		uint64_t pt_prim_lba = 2;
		off_t pt_prim_lba_off = pt_prim_lba * logical_block_size;
		if ( pwriteall(fd, partition_table, partition_table_size,
		               pt_prim_lba_off) < partition_table_size )
		{
			command_error("%s: %s: write", argv[0], name);
			free(partition_table);
			scan_device();
			return;
		}
		uint64_t pt_alt_lba = last_lba - partition_table_sectors;
		off_t pt_alt_lba_off = pt_alt_lba * logical_block_size;
		if ( pwriteall(fd, partition_table, partition_table_size,
		               pt_alt_lba_off) < partition_table_size )
		{
			command_error("%s: %s: write", argv[0], name);
			free(partition_table);
			scan_device();
			return;
		}
		free(partition_table);
		struct gpt gpt;
		memset(&gpt, 0, sizeof(gpt));
		memcpy(gpt.signature, "EFI PART", 8);
		gpt.revision = 0x00010000;
		gpt.header_size = sizeof(gpt) - sizeof(gpt.reserved1);
		gpt.header_crc32 = 0;
		gpt.reserved0 = 0;
		gpt.my_lba = 1;
		gpt.alternate_lba = last_lba;
		gpt.first_usable_lba = pt_prim_lba + partition_table_sectors;
		gpt.last_usable_lba = pt_alt_lba - 1;
		arc4random_buf(&gpt.disk_guid, sizeof(gpt.disk_guid));
		gpt.partition_entry_lba = pt_prim_lba;
		gpt.number_of_partition_entries = partition_table_length;
		gpt.size_of_partition_entry = sizeof(struct gpt_partition);
		gpt.partition_entry_array_crc32 = partition_table_checksum;
		header_size = gpt.header_size;
		gpt_encode(&gpt);
		gpt.header_crc32 = htole32(gpt_crc32(&gpt, header_size));
		if ( pwriteall(fd, &gpt, sizeof(gpt), gpt_off) < sizeof(gpt) )
		{
			command_error("%s: %s: write", argv[0], name);
			scan_device();
			return;
		}
		gpt_decode(&gpt);
		gpt.header_crc32 = 0;
		gpt.reserved0 = 0;
		gpt.my_lba = last_lba;
		gpt.alternate_lba = 1;
		gpt.partition_entry_lba = pt_alt_lba;
		header_size = gpt.header_size;
		gpt_encode(&gpt);
		gpt.header_crc32 = htole32(gpt_crc32(&gpt, header_size));
		if ( pwriteall(fd, &gpt, sizeof(gpt), alt_off) < sizeof(gpt) )
		{
			command_error("%s: %s: write", argv[0], name);
			scan_device();
			return;
		}
		if ( UINT32_MAX < sector_count )
			sector_count = UINT32_MAX;
		struct mbr mbr;
		memset(&mbr, 0, sizeof(mbr));
		mbr.signature[0] = 0x55;
		mbr.signature[1] = 0xAA;
		struct mbr_partition p;
		memset(&p, 0, sizeof(p));
		p.flags = 0;
		p.start_head = 0; // TODO: This.
		p.start_sector_cylinder = 0; // TODO: This.
		p.system_id = 0xEE;
		p.end_head = 0; // TODO: This.
		p.end_sector_cylinder = 0; // TODO: This.
		p.start_sector = 1;
		p.total_sectors = sector_count - p.start_sector;
		mbr_partition_encode(&p);
		memcpy(&mbr.partitions[0], &p, sizeof(p));
		if ( pwriteall(fd, &mbr, sizeof(mbr), 0) < sizeof(mbr) )
		{
			command_error("%s: %s: write", argv[0], name);
			scan_device();
			return;
		}
	}
	else
	{
		command_errorx("%s: Unrecognized partition table type `%s'", argv[0], type);
		return;
	}
	if ( fsync(fd) < 0 )
	{
		command_error("%s: %s: sync", argv[0], name);
		scan_device();
		return;
	}
	// TODO: Rescan this device.
	switch_device(current_hd);
}


static void on_mount(size_t argc, char** argv)
{
	if ( argc < 2 )
	{
		command_errorx("%s: No partition specified", argv[0]);
		return;
	}
	struct partition* part;
	if ( !lookup_partition_by_string(&part, argv[0], argv[1]) )
		return;
	if ( argc < 3 )
	{
		command_errorx("%s: Mountpoint or 'no' wasn't specified", argv[0]);
		return;
	}
	char* mountpoint = argv[2];
	if ( !strcmp(mountpoint, "no") )
	{
		if ( !remove_blockdevice_from_fstab(&part->bdev) )
		{
			command_error("%s: %s: Failed to remove partition",
			              argv[0], fstab_path);
			return;
		}
	}
	else
	{
		if ( !verify_mountpoint(mountpoint) )
		{
			command_errorx("%s: Invalid mountpoint `%s'.", argv[0], mountpoint);
			return;
		}
		simplify_mountpoint(mountpoint);
		if ( !part->bdev.fs || !part->bdev.fs->driver )
		{
			const char* name = device_name(part->path);
			printf("Warning: `%s' is not a mountable filesystem.\n", name);
		}
		if ( !add_blockdevice_to_fstab(&part->bdev, mountpoint) )
		{
			command_error("%s: %s: Failed to remove partition",
			              argv[0], fstab_path);
			return;
		}
	}
}

static void on_quit(size_t argc, char** argv)
{
	(void) argc;
	(void) argv;
	quitting = true;
}

static void on_rmpart(size_t argc, char** argv)
{
	if ( argc < 2 )
	{
		command_errorx("%s: No partition specified", argv[0]);
		return;
	}
	struct partition* part;
	if ( !lookup_partition_by_string(&part, argv[0], argv[1]) )
		return;
	bool ok = false;
	if ( part->type == PARTITION_TYPE_EXTENDED )
	{
		bool has_logical = false;
		for ( size_t i = 0; i < current_pt->partitions_count; i++ )
		{
			if ( current_pt->partitions[i]->type != PARTITION_TYPE_LOGICAL )
				continue;
			has_logical = true;
			break;
		}
		ok = !has_logical;
	}
	if ( interactive && !ok )
	{
		const char* name = device_name(part->path);
		// TODO: Use the name and mount point of the partition if such!
		if ( part->type == PARTITION_TYPE_EXTENDED )
		{
			textf("WARNING: This will \e[91mERASE ALL PARTITIONS\e[m on the "
			      "extended partition \e[93m%s\e[m!\n", name);
			for ( size_t i = 0; i < current_pt->partitions_count; i++ )
			{
				struct partition* logic = current_pt->partitions[i];
				if ( logic->type != PARTITION_TYPE_LOGICAL )
					continue;
				const char* logic_name = device_name(logic->path);
				textf("WARNING: This will \e[91mERASE ALL DATA\e[m on the "
				      "partition \e[93m%s\e[m!\n", logic_name);
			}
		}
		else
			textf("WARNING: This will \e[91mERASE ALL DATA\e[m on the "
			      "partition \e[93m%s\e[m!\n", name);
		// TODO: Warn if GPT require attribute is set.
		while ( true )
		{
			char answer[32];
			prompt(answer, sizeof(answer),
			       "Confirm partition deletion? (yes/no)", "no");
			if ( strcmp(answer, "no") == 0 )
			{
				printf("(Aborted partition deletion)\n");
				return;
			}
			if ( strcmp(answer, "yes") == 0 )
				break;
		}
	}
	// TODO: Ensure the partition is not in use!
	if ( part->type == PARTITION_TYPE_EXTENDED )
	{
		for ( size_t i = 0; i < current_pt->partitions_count; i++ )
		{
			struct partition* logic = current_pt->partitions[i];
			if ( logic->type != PARTITION_TYPE_LOGICAL )
				continue;
			if ( !remove_partition_device(logic->path) )
			{
				command_error("%s: Failed to remove partition device: %s",
						      argv[0], logic->path);
				return;
			}
			if ( !remove_blockdevice_from_fstab(&logic->bdev) )
			{
				command_error("%s: %s: Failed to remove partition", argv[0],
				              fstab_path);
				return;
			}
		}
	}
	if ( !remove_partition_device(part->path) )
	{
		command_error("%s: Failed to remove partition device: %s",
		              argv[0], part->path);
		return;
	}
	if ( !remove_blockdevice_from_fstab(&part->bdev) )
	{
		command_error("%s: %s: Failed to remove partition", argv[0], fstab_path);
		// TODO: Recreate the partition.
		return;
	}
	unsigned int part_index = part->index;
	size_t renumbered_partitions = 0;
	if ( current_pt_type == PARTITION_TABLE_TYPE_MBR &&
	     part->type == PARTITION_TYPE_LOGICAL )
	{
		assert(5 <= part_index);
		renumbered_partitions = part_index;
		for ( size_t i = 0; i < current_pt->partitions_count; i++ )
		{
			if ( current_pt->partitions[i]->index <= part_index )
				continue;
			remove_partition_device(current_pt->partitions[i]->path);
		}
		struct mbr_partition_table* mbrpt =
			(struct mbr_partition_table*) current_pt->raw_partition_table;
		size_t ebr_i = part_index - 5;
		off_t ebr_off = 0;
		struct mbr ebr;
		struct mbr_partition p;
		if ( ebr_i == 0 && mbrpt->ebr_chain_count == 1 )
		{
			ebr_off = mbrpt->ebr_chain[0].offset;
			memset(&ebr, 0, sizeof(ebr));
			ebr.signature[0] = 0x55;
			ebr.signature[1] = 0xAA;
		}
		else if ( ebr_i == 0 )
		{
			ebr_off = mbrpt->ebr_chain[0].offset;
			off_t dist = mbrpt->ebr_chain[1].offset - mbrpt->ebr_chain[0].offset;
			memcpy(&ebr, &mbrpt->ebr_chain[1].ebr, sizeof(ebr));
			memcpy(&p, ebr.partitions[0], sizeof(p));
			mbr_partition_decode(&p);
			p.start_sector += dist / current_hd->logical_block_size;
			mbr_partition_encode(&p);
			memcpy(&ebr.partitions[0], &p, sizeof(p));
		}
		else
		{
			memcpy(&ebr, &mbrpt->ebr_chain[ebr_i].ebr, sizeof(ebr));
			memcpy(&p, ebr.partitions[1], sizeof(p));
			ebr_off = mbrpt->ebr_chain[ebr_i - 1].offset;
			memcpy(&ebr, &mbrpt->ebr_chain[ebr_i - 1].ebr, sizeof(ebr));
			memcpy(&ebr.partitions[1], &p, sizeof(p));
		}
		// TODO: Partitions may be reordered.
		if ( pwriteall(current_hd->fd, &ebr, sizeof(ebr), ebr_off) < sizeof(ebr) )
		{
			command_error("%s: %s: write", argv[0], device_name(current_hd->path));
			scan_device();
			return;
		}
		if ( fsync(current_hd->fd) < 0 )
		{
			command_error("%s: %s: sync", argv[0], device_name(current_hd->path));
			scan_device();
			return;
		}
	}
	else if ( current_pt_type == PARTITION_TABLE_TYPE_MBR )
	{
		assert(0 < part_index);
		assert(part_index <= 4);
		struct mbr_partition_table* mbrpt =
			(struct mbr_partition_table*) current_pt->raw_partition_table;
		struct mbr mbr;
		memcpy(&mbr, &mbrpt->mbr, sizeof(mbr));
		memset(&mbr.partitions[part_index - 1], 0, sizeof(struct mbr_partition));
		if ( pwriteall(current_hd->fd, &mbr, sizeof(mbr), 0) < sizeof(mbr) )
		{
			command_error("%s: %s: write", argv[0], device_name(current_hd->path));
			scan_device();
			return;
		}
		if ( fsync(current_hd->fd) < 0 )
		{
			command_error("%s: %s: sync", argv[0], device_name(current_hd->path));
			scan_device();
			return;
		}
	}
	else if ( current_pt_type == PARTITION_TABLE_TYPE_GPT )
	{
		struct gpt_partition_table* gptpt =
			(struct gpt_partition_table*) current_pt->raw_partition_table;
		struct gpt gpt;
		memcpy(&gpt, &gptpt->gpt, sizeof(gpt));
		gpt_decode(&gpt);
		size_t poff = (part_index - 1) * (size_t) gpt.size_of_partition_entry;
		memset(gptpt->rpt + poff, 0, sizeof(struct gpt_partition));
		if ( !gpt_update(argv[0], gptpt) )
			return;
	}
	else
	{
		command_errorx("%s: %s: Partition scheme not supported", argv[0],
		               device_name(current_hd->path));
		return;
	}
	scan_device();
	printf("(Deleted %sp%u)\n", device_name(current_hd->path), part_index);
	if ( current_pt && renumbered_partitions )
	{
		for ( size_t i = 0; i < current_pt->partitions_count; i++ )
		{
			if ( current_pt->partitions[i]->index < renumbered_partitions )
				continue;
			struct partition* p = current_pt->partitions[i];
			if ( !create_partition_device(argv[0], p) )
				return;
		}
	}
}

static void on_rmtable(size_t argc, char** argv)
{
	(void) argc;
	(void) argv;
	// TODO: We shouldn't do this either if we recognize a filesystem on the
	//       raw device itself.
	if ( current_pt_type != PARTITION_TABLE_TYPE_NONE && interactive )
	{
		const char* name = device_name(current_hd->path);
		textf("WARNING: This will \e[91mERASE ALL DATA\e[m on the device "
		      "\e[93m%s\e[m!\n", name);
		// TODO: List all the partitions?
		// TODO: Use the name and mount point of the partition if such!
		if ( current_pt && 0 < current_pt->partitions_count )
			textf("WARNING: Device \e[93m%s\e[m \e[91mHAS PARTITIONS\e[m!\n",
			      name);
		while ( true )
		{
			char answer[32];
			prompt(answer, sizeof(answer),
			       "Confirm erase partition table? (yes/no)", "no");
			if ( strcmp(answer, "no") == 0 )
			{
				printf("(Aborted partition table erase)\n");
				return;
			}
			if ( strcmp(answer, "yes") == 0 )
				break;
		}
	}
	for ( size_t i = 0; current_pt && i < current_pt->partitions_count; i++ )
	{
		struct partition* part = current_pt->partitions[i];
		if ( !remove_partition_device(part->path) )
		{
			command_error("%s: Failed to remove partition device: %s",
					      argv[0], part->path);
			return;
		}
		if ( !remove_blockdevice_from_fstab(&part->bdev) )
		{
			command_error("%s: %s: Failed to remove partitions",
			              argv[0], fstab_path);
			// TODO: Recreate the partition.
			return;
		}
	}
	remove_partition_devices(current_hd->path);
	// TODO: Assert logical_block_size fits in size_t.
	size_t block_size = current_hd->logical_block_size;
	unsigned char* zeroes = (unsigned char*) calloc(1, block_size);
	if ( !zeroes )
	{
		command_error("malloc");
		return;
	}
	off_t sector_0 = 0; // MBR & GPT
	off_t sector_1 = block_size; // GPT
	// TODO: Overflow: Ensure underflow is not possible.
	off_t sector_m1 = current_hd->st.st_size - block_size; // GPT
	if ( pwriteall(current_hd->fd, zeroes, block_size, sector_0) < block_size ||
	     pwriteall(current_hd->fd, zeroes, block_size, sector_1) < block_size ||
	     pwriteall(current_hd->fd, zeroes, block_size, sector_m1) < block_size )
	{
		command_error("%s: %s: write", argv[0], device_name(current_hd->path));
		free(zeroes);
		return;
	}
	free(zeroes);
	if ( fsync(current_hd->fd) < 0 )
	{
		command_error("%s: %s: sync", argv[0], device_name(current_hd->path));
		return;
	}
	scan_device();
}

static void on_sh(size_t argc, char** argv)
{
	(void) argc;
	sigset_t oldset, sigttou;
	sigemptyset(&sigttou);
	sigaddset(&sigttou, SIGTTOU);
	pid_t child_pid = fork();
	if ( child_pid < 0 )
	{
		command_error("%s: fork", argv[0]);
		return;
	}
	if ( child_pid == 0 )
	{
		setpgid(0, 0);
		sigprocmask(SIG_BLOCK, &sigttou, &oldset);
		tcsetpgrp(0, getpgid(0));
		sigprocmask(SIG_SETMASK, &oldset, NULL);
		const char* subargv[] = { "sh", NULL };
		execvp(subargv[0], (char* const*) subargv);
		warn("%s", subargv[0]);
		_exit(127);
	}
	int code;
	waitpid(child_pid, &code, 0);
	sigprocmask(SIG_BLOCK, &sigttou, &oldset);
	tcsetpgrp(0, getpgid(0));
	sigprocmask(SIG_SETMASK, &oldset, NULL);
	scan_device();
}

// TODO: mkfs command.
static const struct command commands[] =
{
	{ "!", on_sh, "as" },
	{ "!man", on_man, "as" },
	{ "?", on_help, "a" },
	{ "device", on_device, "" },
	{ "devices", on_devices, "" },
	{ "d", on_device, "a" },
	{ "ds", on_devices, "a" },
	{ "e", on_quit, "a" },
	{ "exit", on_quit, "" },
	{ "fsck", on_fsck, "dtp" },
	{ "help", on_help, "" },
	{ "ls", on_ls, "dt" },
	{ "man", on_man, "s" },
	{ "mkpart", on_mkpart, "dt" },
	{ "mktable", on_mktable, "d" },
	{ "mount", on_mount, "dtp" },
	{ "q", on_quit, "a" },
	{ "quit", on_quit, "a" },
	{ "rmpart", on_rmpart, "dtp" },
	{ "rmtable", on_rmtable, "d" },
	{ "sh", on_sh, "s" },
};
static const size_t commands_count = sizeof(commands) / sizeof(commands[0]);

void execute(char* cmd)
{
	size_t argc = 0;
	char* argv[256];
	split_arguments(cmd, &argc, argv, 255);
	if ( argc < 1 )
		return;
	argv[argc] = NULL;
	for ( size_t i = 0; i < commands_count; i++ )
	{
		if ( strcmp(argv[0], commands[i].name) != 0 )
			continue;
		if ( strchr(commands[i].flags, 'd') && !current_hd )
		{
			if ( !interactive )
				errx(1, "No device specified");
			fprintf(stderr, "Use the `device' command first to specify a device.\n");
			fprintf(stderr, "You can list devices with the `devices' command.\n");
			return;
		}
		if ( strchr(commands[i].flags, 't') && !current_pt )
		{
			if ( current_pt_type == PARTITION_TABLE_TYPE_NONE )
				printf("%s: No partition table found\n",
					   device_name(current_hd->path));
			else if ( current_pt_type == PARTITION_TABLE_TYPE_UNKNOWN )
				printf("%s: No partition table recognized\n",
					   device_name(current_hd->path));
			else
				command_errorx("%s: %s: Partition table not loaded",
					           argv[0], device_name(current_hd->path));
			return;
		}
		commands[i].function(argc, argv);
		return;
	}
	if ( !interactive )
		errx(1, "unrecognized command `%s'", argv[0]);
	fprintf(stderr, "Unrecognized command `%s'. "
	                "Try `help' for more information.\n", argv[0]);
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
	fprintf(fp, "Edit disk partition tables\n");
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
		else if ( !strncmp(arg, "--fstab=", strlen("--fstab=")) )
			fstab_path = arg + strlen("--fstab=");
		else if ( !strcmp(arg, "--fstab") )
		{
			if ( i + 1 == argc )
			{
				warn( "option '--fstab' requires an argument");
				fprintf(stderr, "Try `%s --help' for more information.\n", argv[0]);
				exit(125);
			}
			fstab_path = argv[i+1];
			argv[++i] = NULL;
		}
		else
		{
			fprintf(stderr, "%s: unknown option: %s\n", argv0, arg);
			help(stderr, argv0);
			exit(1);
		}
	}

	compact_arguments(&argc, &argv);

	interactive = isatty(0) && isatty(1);

	if ( !devices_open_all(&hds, &hds_count) )
		err(1, "iterating devices");

	qsort(hds, hds_count, sizeof(struct harddisk*), harddisk_compare_path);

	if ( 1 <= hds_count )
		switch_device(hds[0]);

	char* line = NULL;
	size_t line_size = 0;
	ssize_t line_length;
	while ( !quitting )
	{
		if ( interactive )
		{
			printf("\e[93m(%s)\e[m ",
			       current_hd ? device_name(current_hd->path) : "disked");
			fflush(stdout);
		}

		if ( (line_length = getline(&line, &line_size, stdin)) < 0 )
		{
			if ( interactive )
				printf("\n");
			break;
		}

		if ( line_length && line[line_length-1] == '\n' )
			line[--line_length] = '\0';

		execute(line);
	}
	free(line);

	if ( ferror(stdin) )
		err(1, "getline");

	for ( size_t i = 0; i < hds_count; i++ )
		harddisk_close(hds[i]);
	free(hds);
}
