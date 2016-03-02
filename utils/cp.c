/*
 * Copyright (c) 2011, 2012, 2013, 2014, 2016 Jonas 'Sortie' Termansen.
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
 * cp.c
 * Copy files and directories.
 */

#include <sys/stat.h>
#include <sys/types.h>

#include <dirent.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <timespec.h>
#include <unistd.h>

const char* BaseName(const char* path)
{
	size_t len = strlen(path);
	while ( len-- )
		if ( path[len] == '/' )
			return path + len + 1;
	return path;
}

char* AddElemToPath(const char* path, const char* elem)
{
	size_t pathlen = strlen(path);
	size_t elemlen = strlen(elem);
	if ( pathlen && path[pathlen-1] == '/' )
	{
		char* ret = (char*) malloc(sizeof(char) * (pathlen + elemlen + 1));
		stpcpy(stpcpy(ret, path), elem);
		return ret;
	}
	char* ret = (char*) malloc(sizeof(char) * (pathlen + 1 + elemlen + 1));
	stpcpy(stpcpy(stpcpy(ret, path), "/"), elem);
	return ret;
}

static const int FLAG_RECURSIVE = 1 << 0;
static const int FLAG_VERBOSE = 1 << 1;
static const int FLAG_TARGET_DIR = 1 << 2;
static const int FLAG_NO_TARGET_DIR = 1 << 3;
static const int FLAG_UPDATE = 1 << 4;
static const int FLAG_FORCE = 1 << 5;

enum symbolic_dereference
{
	SYMBOLIC_DEREFERENCE_NONE,
	SYMBOLIC_DEREFERENCE_ARGUMENTS,
	SYMBOLIC_DEREFERENCE_ALWAYS,
	SYMBOLIC_DEREFERENCE_DEFAULT,
};

bool CopyFileContents(int srcfd, const char* srcpath,
                      int dstfd, const char* dstpath, int flags);
bool CopyDirectoryContents(int srcfd, const char* srcpath,
                           int dstfd, const char* dstpath, int flags,
                           enum symbolic_dereference symbolic_dereference);
bool CopyToDest(int srcdirfd, const char* srcrel, const char* srcpath,
                int dstdirfd, const char* dstrel, const char* dstpath,
                int flags, enum symbolic_dereference symbolic_dereference);
bool CopyIntoDirectory(int srcdirfd, const char* srcrel, const char* srcpath,
                       int dstdirfd, const char* dstrel, const char* dstpath,
                       int flags, enum symbolic_dereference symbolic_dereference);
bool CopyAmbigious(int srcdirfd, const char* srcrel, const char* srcpath,
                   int dstdirfd, const char* dstrel, const char* dstpath,
                   int flags, enum symbolic_dereference symbolic_dereference);

bool CopyFileContents(int srcfd, const char* srcpath,
                      int dstfd, const char* dstpath, int flags)
{
	struct stat srcst, dstst;
	if ( fstat(srcfd, &srcst) )
	{
		error(0, errno, "stat: %s", srcpath);
		return false;
	}
	if ( fstat(dstfd, &dstst) )
	{
		error(0, errno, "stat: %s", dstpath);
		return false;
	}
	if ( srcst.st_dev == dstst.st_dev && srcst.st_ino == dstst.st_ino )
	{
		error(0, 0, "`%s' and `%s' are the same file", srcpath, dstpath);
		return false;
	}
	if ( S_ISDIR(dstst.st_mode) )
	{
		error(0, 0, "cannot overwrite directory `%s' with non-directory",
	                dstpath);
		return false;
	}
	if ( lseek(srcfd, 0, SEEK_SET) )
	{
		error(0, errno, "can't seek: %s", srcpath);
		return false;
	}
	if ( lseek(dstfd, 0, SEEK_SET) )
	{
		error(0, errno, "can't seek: %s", dstpath);
		return false;
	}
	if ( flags & FLAG_VERBOSE )
		printf("`%s' -> `%s'\n", srcpath, dstpath);
	ftruncate(dstfd, srcst.st_size);
	size_t BUFFER_SIZE = 16 * 1024;
	uint8_t buffer[BUFFER_SIZE];
	while ( true )
	{
		ssize_t numbytes = read(srcfd, buffer, BUFFER_SIZE);
		if ( numbytes == 0 )
			break;
		if ( numbytes < 0 )
		{
			error(0, errno, "read: %s", srcpath);
			return false;
		}
		size_t sofar = 0;
		while ( sofar < (size_t) numbytes )
		{
			ssize_t amount = write(dstfd, buffer + sofar, numbytes - sofar);
			if ( amount <= 0 )
			{
				error(0, errno, "write: %s", dstpath);
				return false;
			}
			sofar += amount;
		}
	}
	return true;
}

bool CopyDirectoryContentsInner(DIR* srcdir, const char* srcpath,
                                DIR* dstdir, const char* dstpath, int flags,
                                enum symbolic_dereference symbolic_dereference)
{
	if ( flags & FLAG_VERBOSE )
		printf("`%s' -> `%s'\n", srcpath, dstpath);
	bool ret = true;
	struct dirent* entry;
	while ( (entry = readdir(srcdir)) )
	{
		const char* name = entry->d_name;
		if ( !strcmp(name, ".") || !strcmp(name, "..") )
			continue;
		char* srcpath_new = AddElemToPath(srcpath, name);
		char* dstpath_new = AddElemToPath(dstpath, name);
		bool ok = CopyToDest(dirfd(srcdir), name, srcpath_new,
		                     dirfd(dstdir), name, dstpath_new, flags,
                             symbolic_dereference);
		free(srcpath_new);
		free(dstpath_new);
		if ( !ok )
			ret = false;
	}
	return ret;
}

bool CopyDirectoryContentsOuter(int srcfd, const char* srcpath,
                                int dstfd, const char* dstpath, int flags,
                                enum symbolic_dereference symbolic_dereference)
{
	struct stat srcst, dstst;
	if ( fstat(srcfd, &srcst) < 0 )
	{
		error(0, errno, "stat: %s", srcpath);
		return false;
	}
	if ( fstat(dstfd, &dstst) < 0 )
	{
		error(0, errno, "stat: %s", dstpath);
		return false;
	}
	if ( srcst.st_dev == dstst.st_dev && srcst.st_ino == dstst.st_ino )
	{
		error(0, 0, "error: `%s' and `%s' are the same file", srcpath, dstpath);
		return false;
	}
	DIR* srcdir = fdopendir(srcfd);
	if ( !srcdir )
		return error(0, errno, "fdopendir()"), false;
	DIR* dstdir = fdopendir(dstfd);
	if ( !dstdir )
		return error(0, errno, "fdopendir()"), closedir(srcdir), false;
	bool ret = CopyDirectoryContentsInner(srcdir, srcpath, dstdir, dstpath,
	                                      flags, symbolic_dereference);
	closedir(dstdir);
	closedir(srcdir);
	return ret;
}

bool CopyDirectoryContents(int srcfd, const char* srcpath,
                           int dstfd, const char* dstpath, int flags,
                           enum symbolic_dereference symbolic_dereference)
{
	int srcfd_copy = dup(srcfd);
	if ( srcfd_copy < 0 )
		return error(0, errno, "dup"), false;
	int dstfd_copy = dup(dstfd);
	if ( dstfd_copy < 0 )
		return error(0, errno, "dup"), close(srcfd_copy), false;
	bool ret = CopyDirectoryContentsOuter(srcfd_copy, srcpath,
	                                      dstfd_copy, dstpath, flags,
	                                      symbolic_dereference);
	close(dstfd_copy);
	close(srcfd_copy);
	return ret;
}

bool CopyToDest(int srcdirfd, const char* srcrel, const char* srcpath,
                int dstdirfd, const char* dstrel, const char* dstpath,
                int flags, enum symbolic_dereference symbolic_dereference)
{
	struct stat srcst;
	bool ret = false;
	int srcfd_flags = O_RDONLY;
	if ( symbolic_dereference == SYMBOLIC_DEREFERENCE_NONE )
		srcfd_flags |= O_SYMLINK_NOFOLLOW;
	int srcfd = openat(srcdirfd, srcrel, O_RDONLY | srcfd_flags);
	if ( srcfd < 0 )
	{
		error(0, errno, "%s", srcpath);
		return false;
	}
	if ( fstat(srcfd, &srcst) )
	{
		error(0, errno, "stat: %s", srcpath);
		return close(srcfd), false;
	}
	if ( symbolic_dereference == SYMBOLIC_DEREFERENCE_ARGUMENTS )
		symbolic_dereference = SYMBOLIC_DEREFERENCE_NONE;
	// TODO: I'm really not satisfied with the atomicity properties in this case.
	if ( S_ISLNK(srcst.st_mode) )
	{
		// TODO: How to handle the update flag in this case?
		struct stat dstst;
		int dstst_ret = fstatat(dstdirfd, dstrel, &dstst, AT_SYMLINK_NOFOLLOW);
		if ( dstst_ret < 0 && errno != ENOENT )
		{
			error(0, errno, "%s", dstpath);
			return close(srcfd), false;
		}
		bool already_is_symlink = false;
		if ( dstst_ret == 0 )
		{
			if ( S_ISLNK(dstst.st_mode) )
			{
				already_is_symlink = true;
			}
			else if ( S_ISDIR(dstst.st_mode) )
			{
				error(0, 0, "cannot overwrite directory `%s' with non-directory",
	                  dstpath);
				return close(srcfd), false;
			}
			else
			{
				if ( unlinkat(dstdirfd, dstrel, 0) == 0 )
				{
					if ( flags & FLAG_VERBOSE )
						printf("removed `%s'\n", dstpath);
				}
				else if ( errno != ENOENT )
				{
					error(0, errno, "unlink: %s", dstpath);
					return close(srcfd), false;
				}
			}
		}
		if ( !already_is_symlink )
		{
			if ( symlinkat("", dstdirfd, dstrel) < 0 )
			{
				error(0, errno, "creating symlink: %s", dstpath);
				return close(srcfd), false;
			}
		}
		int dstfd = openat(dstdirfd, dstrel, O_WRONLY | O_SYMLINK_NOFOLLOW);
		if ( dstfd < 0 )
		{
			error(0, errno, "creating and opening symlink: %s", dstpath);
			return close(srcfd), false;
		}
		if ( fstat(dstfd, &dstst) < 0 )
		{
			error(0, errno, "stat of symlink: %s", dstpath);
			return close(dstfd), close(srcfd), false;
		}
		if ( !S_ISLNK(dstst.st_mode) )
		{
			error(0, errno, "unexpectedly not a symlink: %s", dstpath);
			return close(dstfd), close(srcfd), false;
		}
		ret = CopyFileContents(srcfd, srcpath, dstfd, dstpath, flags);
		close(dstfd);
	}
	else if ( S_ISDIR(srcst.st_mode) )
	{
		if ( !(flags & FLAG_RECURSIVE) )
		{
			error(0, 0, "omitting directory `%s'", srcpath);
				return close(srcfd), false;
		}
		int dstfd = openat(dstdirfd, dstrel, O_RDONLY | O_DIRECTORY);
		if ( dstfd < 0 )
		{
			if ( errno != ENOENT )
			{
				error(0, errno, "%s", dstpath);
				return close(srcfd), false;
			}
			if ( mkdirat(dstdirfd, dstrel, srcst.st_mode & 03777) )
			{
				error(0, errno, "cannot create directory `%s'", dstpath);
				return close(srcfd), false;
			}
			if ( (dstfd = openat(dstdirfd, dstrel, O_RDONLY | O_DIRECTORY)) < 0 )
			{
				error(0, errno, "%s", dstpath);
				return close(srcfd), false;
			}
		}
		ret = CopyDirectoryContents(srcfd, srcpath, dstfd, dstpath, flags,
		                            symbolic_dereference);
		close(dstfd);
	}
	else
	{
		if ( flags & FLAG_UPDATE )
		{
			struct stat dstst;
			if ( fstatat(dstdirfd, dstrel, &dstst, 0) == 0 && S_ISREG(dstst.st_mode) )
			{
				if ( timespec_le(srcst.st_mtim, dstst.st_mtim) )
					return close(srcfd), true;
			}
		}

		int oflags = O_WRONLY | O_CREAT;
		mode_t omode = srcst.st_mode & 03777;
		int dstfd = openat(dstdirfd, dstrel, oflags, omode);
		if ( dstfd < 0 &&
		     (flags & FLAG_FORCE) &&
		     faccessat(dstdirfd, dstrel, F_OK, AT_SYMLINK_NOFOLLOW) == 0 )
		{
			if ( unlinkat(dstdirfd, dstrel, 0) < 0 )
			{
				error(0, errno, "%s", dstpath);
				return close(srcfd), false;
			}
			dstfd = openat(dstdirfd, dstrel, oflags, omode);
		}
		if ( dstfd < 0 )
		{
			error(0, errno, "%s", dstpath);
			return close(srcfd), false;
		}
		ret = CopyFileContents(srcfd, srcpath, dstfd, dstpath, flags);
		close(dstfd);
	}
	close(srcfd);
	return ret;
}

bool CopyIntoDirectory(int srcdirfd, const char* srcrel, const char* srcpath,
                       int dstdirfd, const char* dstrel, const char* dstpath,
                       int flags, enum symbolic_dereference symbolic_dereference)
{
	const char* src_basename = BaseName(srcrel);
	int dstfd = openat(dstdirfd, dstrel, O_RDONLY | O_DIRECTORY);
	if ( dstfd < 0 )
	{
		error(0, errno, "%s", dstpath);
		return false;
	}
	char* dstpath_new = AddElemToPath(dstpath, src_basename);
	bool ret = CopyToDest(srcdirfd, srcrel, srcpath,
	                      dstfd, src_basename, dstpath_new,
                          flags, symbolic_dereference);
	free(dstpath_new);
	return ret;
}

bool CopyAmbigious(int srcdirfd, const char* srcrel, const char* srcpath,
                   int dstdirfd, const char* dstrel, const char* dstpath,
                   int flags, enum symbolic_dereference symbolic_dereference)
{
	struct stat dstst;
	if ( fstatat(dstdirfd, dstrel, &dstst, 0) < 0 )
	{
		if ( errno != ENOENT )
		{
			error(0, errno, "%s", dstpath);
			return false;
		}
		dstst.st_mode = S_IFREG;
	}
	if ( S_ISDIR(dstst.st_mode) )
		return CopyIntoDirectory(srcdirfd, srcrel, srcpath,
	                             dstdirfd, dstrel, dstpath,
		                         flags, symbolic_dereference);
	else
		return CopyToDest(srcdirfd, srcrel, srcpath,
		                  dstdirfd, dstrel, dstpath,
		                  flags, symbolic_dereference);
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
	fprintf(fp, "Usage: %s [OPTION]... [-T] SOURCE DEST\n", argv0);
	fprintf(fp, "  or:  %s [OPTION]... SOURCE... DIRECTORY\n", argv0);
	fprintf(fp, "  or:  %s [OPTION]... -t DIRECTORY SOURCE...\n", argv0);
#ifdef CP_PRETEND_TO_BE_INSTALL
	fprintf(fp, "Copy files and set attributes.\n");
#else
	fprintf(fp, "Copy SOURCE to DEST, or multiple SOURCE(s) to DIRECTORY.\n");
#endif
}

static void version(FILE* fp, const char* argv0)
{
	fprintf(fp, "%s (Sortix) %s\n", argv0, VERSIONSTR);
}

int main(int argc, char* argv[])
{
	const char* argv0 = argv[0];
	int flags = 0;
	const char* target_directory = NULL;
	const char* preserve_list = NULL;
	enum symbolic_dereference symbolic_dereference = SYMBOLIC_DEREFERENCE_DEFAULT;
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
#ifdef CP_PRETEND_TO_BE_INSTALL
			case 'b': /* ignored */ break;
			case 'c': /* ignored */ break;
			case 'C': /* ignored */ break;
			case 'd': /* ignored */ break;
			case 'g': if ( *(arg + 1) ) arg = "g"; else if ( i + 1 != argc ) argv[++i] = NULL; break;
			case 'm': if ( *(arg + 1) ) arg = "m"; else if ( i + 1 != argc ) argv[++i] = NULL; break;
			case 'o': if ( *(arg + 1) ) arg = "o"; else if ( i + 1 != argc ) argv[++i] = NULL; break;
			case 's': /* ignored */ break;
#endif
			case 'f': flags |= FLAG_FORCE; break;
			case 'H': symbolic_dereference = SYMBOLIC_DEREFERENCE_ARGUMENTS; break;
			case 'L': symbolic_dereference = SYMBOLIC_DEREFERENCE_ALWAYS; break;
			case 'r':
			case 'R': flags |= FLAG_RECURSIVE; break;
			case 'v': flags |= FLAG_VERBOSE; break;
			case 't':
				flags |= FLAG_TARGET_DIR;
				if ( *(arg + 1) )
				{
					target_directory = arg + 1;
				}
				else if ( i + 1 == argc )
				{
					error(0, 0, "option requires an argument -- '%c'", c);
					fprintf(stderr, "Try '%s --help' for more information\n", argv0);
					exit(1);
				}
				else
				{
					target_directory = argv[i+1];
					argv[++i] = NULL;
				}
				arg = "t";
				break;
			case 'T': flags |= FLAG_NO_TARGET_DIR; break;
			case 'u': flags |= FLAG_UPDATE; break;
			case 'p': preserve_list = "mode,ownership,timestamps"; break;
			case 'P': symbolic_dereference = SYMBOLIC_DEREFERENCE_NONE; break;
			default:
#ifdef CP_PRETEND_TO_BE_INSTALL
				fprintf(stderr, "%s (fake): unknown option, ignoring -- '%c'\n", argv0, c);
				continue;
#endif
				fprintf(stderr, "%s: unknown option -- '%c'\n", argv0, c);
				help(stderr, argv0);
				exit(1);
			}
		}
		else if ( !strcmp(arg, "--help") )
			help(stdout, argv0), exit(0);
		else if ( !strcmp(arg, "--version") )
			version(stdout, argv0), exit(0);
		else if ( !strcmp(arg, "--dereference") )
			symbolic_dereference = SYMBOLIC_DEREFERENCE_ALWAYS;
		else if ( !strcmp(arg, "--recursive") )
			flags |= FLAG_RECURSIVE;
		else if ( !strcmp(arg, "--verbose") )
			flags |= FLAG_VERBOSE;
		else if ( !strcmp(arg, "--preserve") )
			preserve_list = "mode,ownership,timestamps";
		else if ( !strncmp(arg, "--preserve=", strlen("--preserve=")) )
			preserve_list = arg + strlen("--preserve=");
		else if ( !strcmp(arg, "--target-directory") )
		{
			if ( i + 1 == argc )
			{
				error(0, 0, "option '--target-directory' requires an argument");
				fprintf(stderr, "Try '%s --help' for more information\n", argv0);
				exit(1);
			}
			target_directory = argv[++i];
			argv[i] = NULL;
			flags |= FLAG_TARGET_DIR;
		}
		else if ( !strncmp(arg, "--target-directory=", strlen("--target-directory=")) )
		{
			target_directory = arg + strlen("--target-directory=");
			flags |= FLAG_TARGET_DIR;
		}
		else if ( !strcmp(arg, "--no-target-directory") )
			flags |= FLAG_NO_TARGET_DIR;
		else if ( !strcmp(arg, "--update") )
			flags |= FLAG_UPDATE;
		else if ( !strcmp(arg, "--no-dereference") )
			symbolic_dereference = SYMBOLIC_DEREFERENCE_NONE;
		else
		{
#ifdef CP_PRETEND_TO_BE_INSTALL
			fprintf(stderr, "%s (fake): unknown option, ignoring: %s\n", argv0, arg);
			continue;
#endif
			fprintf(stderr, "%s: unknown option: %s\n", argv0, arg);
			help(stderr, argv0);
			exit(1);
		}
	}

	if ( (flags & FLAG_TARGET_DIR) && (flags & FLAG_NO_TARGET_DIR) )
		error(1, 0, "cannot combine --target-directory (-t) and --no-target-directory (-T)");

	if ( symbolic_dereference == SYMBOLIC_DEREFERENCE_DEFAULT )
	{
		if ( flags & FLAG_RECURSIVE )
			symbolic_dereference = SYMBOLIC_DEREFERENCE_NONE;
		else
			symbolic_dereference = SYMBOLIC_DEREFERENCE_ALWAYS;
	}

	// TODO: Actually preserve.
	(void) preserve_list;

	compact_arguments(&argc, &argv);

	if ( argc < 2 )
		error(1, 0, "missing file operand");

	if ( flags & FLAG_NO_TARGET_DIR )
	{
		const char* src = argv[1];
		if ( argc < 3 )
			error(1, 0, "missing destination file operand after `%s'", src);
		const char* dst = argv[2];
		if ( 3 < argc )
			error(1, 0, "extra operand `%s'", argv[3]);
		return CopyToDest(AT_FDCWD, src, src, AT_FDCWD, dst, dst, flags,
		                  symbolic_dereference) ? 0 : 1;
	}

	if ( !(flags & FLAG_TARGET_DIR) && argc <= 3 )
	{
		const char* src = argv[1];
		if ( argc < 3 )
			error(1, 0, "missing destination file operand after `%s'", src);
		const char* dst = argv[2];
		if ( 3 < argc )
			error(1, 0, "extra operand `%s'", argv[3]);
		return CopyAmbigious(AT_FDCWD, src, src, AT_FDCWD, dst, dst, flags,
		                     symbolic_dereference) ? 0 : 1;
	}

	if ( !target_directory )
	{
		target_directory = argv[--argc];
		argv[argc] = NULL;
	}

	if ( argc < 2 )
		error(1, 0, "missing file operand");

	bool success = true;
	for ( int i = 1; i < argc; i++ )
	{
		const char* src = argv[i];
		const char* dst = target_directory;
		if ( !CopyIntoDirectory(AT_FDCWD, src, src, AT_FDCWD, dst, dst, flags,
		                        symbolic_dereference) )
			success = false;
	}

	return success ? 0 : 1;
}
