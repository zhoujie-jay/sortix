/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2013.

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

    ext-constants.h
    Constants for the extended filesystem.

*******************************************************************************/

#ifndef EXT_CONSTANTS_H
#define EXT_CONSTANTS_H

const uint16_t EXT2_SUPER_MAGIC = 0xEF53;
const uint16_t EXT2_VALID_FS = 1;
const uint16_t EXT2_ERROR_FS = 2;
const uint16_t EXT2_ERRORS_CONTINUE = 1;
const uint16_t EXT2_ERRORS_RO = 2;
const uint16_t EXT2_ERRORS_PANIC = 3;
const uint32_t EXT2_OS_LINUX = 0;
const uint32_t EXT2_OS_HURD = 1;
const uint32_t EXT2_OS_MASIX = 2;
const uint32_t EXT2_OS_FREEBSD = 3;
const uint32_t EXT2_OS_LITES = 4;
const uint32_t EXT2_GOOD_OLD_REV = 0;
const uint32_t EXT2_DYNAMIC_REV = 1;
const uint16_t EXT2_DEF_RESUID = 0;
const uint16_t EXT2_DEF_RESGID = 0;
const uint16_t EXT2_GOOD_OLD_FIRST_INO = 11;
const uint16_t EXT2_GOOD_OLD_INODE_SIZE = 128;
const uint32_t EXT2_FEATURE_COMPAT_DIR_PREALLOC = 1U << 0U;
const uint32_t EXT2_FEATURE_COMPAT_IMAGIC_INODES = 1U << 1U;
const uint32_t EXT3_FEATURE_COMPAT_HAS_JOURNAL = 1U << 2U;
const uint32_t EXT2_FEATURE_COMPAT_EXT_ATTR = 1U << 3U;
const uint32_t EXT2_FEATURE_COMPAT_RESIZE_INO = 1U << 4U;
const uint32_t EXT2_FEATURE_COMPAT_DIR_INDEX = 1U << 5U;
const uint32_t EXT2_FEATURE_INCOMPAT_COMPRESSION = 1U << 0U;
const uint32_t EXT2_FEATURE_INCOMPAT_FILETYPE = 1U << 1U;
const uint32_t EXT2_FEATURE_INCOMPAT_RECOVER = 1U << 2U;
const uint32_t EXT2_FEATURE_INCOMPAT_JOURNAL_DEV = 1U << 3U;
const uint32_t EXT2_FEATURE_INCOMPAT_META_BG = 1U << 4U;
const uint32_t EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER = 1U << 0U;
const uint32_t EXT2_FEATURE_RO_COMPAT_LARGE_FILE = 1U << 1U;
const uint32_t EXT2_FEATURE_RO_COMPAT_BTREE_DIR = 1U << 2U;
const uint32_t EXT2_LZV1_ALG = 1U << 0U;
const uint32_t EXT2_LZRW3A_ALG = 1U << 1U;
const uint32_t EXT2_GZIP_ALG = 1U << 2U;
const uint32_t EXT2_BZIP2_ALG = 1U << 3U;
const uint32_t EXT2_LZO_ALG = 1U << 4U;
const uint16_t EXT2_S_IFMT = 0xF000;
const uint16_t EXT2_S_IFSOCK = 0xC000;
const uint16_t EXT2_S_IFLNK = 0xA000;
const uint16_t EXT2_S_IFREG = 0x8000;
const uint16_t EXT2_S_IFBLK = 0x6000;
const uint16_t EXT2_S_IFDIR = 0x4000;
const uint16_t EXT2_S_IFCHR = 0x2000;
const uint16_t EXT2_S_IFIFO = 0x1000;
const uint16_t EXT2_S_ISUID = 0x0800;
const uint16_t EXT2_S_ISGID = 0x0400;
const uint16_t EXT2_S_ISVTX = 0x0200;
const uint16_t EXT2_S_IRUSR = 0x0100;
const uint16_t EXT2_S_IWUSR = 0x0080;
const uint16_t EXT2_S_IXUSR = 0x0040;
const uint16_t EXT2_S_IRGRP = 0x0020;
const uint16_t EXT2_S_IWGRP = 0x0010;
const uint16_t EXT2_S_IXGRP = 0x0008;
const uint16_t EXT2_S_IROTH = 0x0004;
const uint16_t EXT2_S_IWOTH = 0x0002;
const uint16_t EXT2_S_IXOTH = 0x0001;
#define EXT2_S_ISSOCK(mode) (((mode) & EXT2_S_IFMT) == EXT2_S_IFSOCK)
#define EXT2_S_ISLNK(mode) (((mode) & EXT2_S_IFMT) == EXT2_S_IFLNK)
#define EXT2_S_ISREG(mode) (((mode) & EXT2_S_IFMT) == EXT2_S_IFREG)
#define EXT2_S_ISBLK(mode) (((mode) & EXT2_S_IFMT) == EXT2_S_IFBLK)
#define EXT2_S_ISDIR(mode) (((mode) & EXT2_S_IFMT) == EXT2_S_IFDIR)
#define EXT2_S_ISCHR(mode) (((mode) & EXT2_S_IFMT) == EXT2_S_IFCHR)
#define EXT2_S_ISFIFO(mode) (((mode) & EXT2_S_IFMT) == EXT2_S_IFIFO)
const uint32_t EXT2_SECRM_FL = 0x00000001U;
const uint32_t EXT2_UNRM_FL = 0x00000002U;
const uint32_t EXT2_COMPR_FL = 0x00000004U;
const uint32_t EXT2_SYNC_FL = 0x00000008U;
const uint32_t EXT2_IMMUTABLE_FL = 0x00000010U;
const uint32_t EXT2_APPEND_FL = 0x00000020U;
const uint32_t EXT2_NODUMP_FL = 0x00000040U;
const uint32_t EXT2_NOATIME_FL = 0x00000080U;
const uint32_t EXT2_DIRTY_FL = 0x00000100U;
const uint32_t EXT2_COMPRBLK_FL = 0x00000200U;
const uint32_t EXT2_NOCOMPR_FL = 0x00000400U;
const uint32_t EXT2_ECOMPR_FL = 0x00000800U;
const uint32_t EXT2_BTREE_FL = 0x00001000U;
const uint32_t EXT2_INDEX_FL = 0x00001000U;
const uint32_t EXT2_IMAGIC_FL = 0x00002000U;
const uint32_t EXT3_JOURNAL_DATA_FL = 0x00004000U;
const uint32_t EXT2_RESERVED_FL = 0x80000000U;
const uint32_t EXT2_ROOT_INO = 2;
const uint8_t EXT2_FT_UNKNOWN = 0;
const uint8_t EXT2_FT_REG_FILE = 1;
const uint8_t EXT2_FT_DIR = 2;
const uint8_t EXT2_FT_CHRDEV = 3;
const uint8_t EXT2_FT_BLKDEV = 4;
const uint8_t EXT2_FT_FIFO = 5;
const uint8_t EXT2_FT_SOCK = 6;
const uint8_t EXT2_FT_SYMLINK = 7;

static inline uint8_t EXT2_FT_OF_MODE(mode_t mode)
{
	if ( EXT2_S_ISREG(mode) )
		return EXT2_FT_REG_FILE;
	if ( EXT2_S_ISDIR(mode) )
		return EXT2_FT_DIR;
	if ( EXT2_S_ISCHR(mode) )
		return EXT2_FT_CHRDEV;
	if ( EXT2_S_ISBLK(mode) )
		return EXT2_FT_BLKDEV;
	if ( EXT2_S_ISFIFO(mode) )
		return EXT2_FT_FIFO;
	if ( EXT2_S_ISSOCK(mode) )
		return EXT2_FT_SOCK;
	if ( EXT2_S_ISLNK(mode) )
		return EXT2_FT_SYMLINK;
	return EXT2_FT_UNKNOWN;
}

#endif
