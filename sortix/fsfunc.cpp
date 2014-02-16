/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012.

    This file is part of Sortix.

    Sortix is free software: you can redistribute it and/or modify it under the
    terms of the GNU General Public License as published by the Free Software
    Foundation, either version 3 of the License, or (at your option) any later
    version.

    Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
    details.

    You should have received a copy of the GNU General Public License along with
    Sortix. If not, see <http://www.gnu.org/licenses/>.

    fsfunc.cpp
    Filesystem related utility functions.

*******************************************************************************/

#include <sys/types.h>

#include <stddef.h>
#include <stdint.h>

#include <sortix/dirent.h>
#include <sortix/stat.h>

#include <sortix/kernel/kernel.h>
#include <sortix/kernel/fsfunc.h>
#include <sortix/kernel/string.h>

#include <string.h>

namespace Sortix {

unsigned char ModeToDT(mode_t mode)
{
	if ( S_ISSOCK(mode) )
		return DT_SOCK;
	if ( S_ISLNK(mode) )
		return DT_LNK;
	if ( S_ISREG(mode) )
		return DT_REG;
	if ( S_ISBLK(mode) )
		return DT_BLK;
	if ( S_ISDIR(mode) )
		return DT_DIR;
	if ( S_ISCHR(mode) )
		return DT_CHR;
	if ( S_ISFIFO(mode) )
		return DT_FIFO;
	return DT_UNKNOWN;
}

// '' -> '' ''
// '/' -> '' '/'
// '///' -> '' '///'
// '.' -> '' '.'
// 'test' -> '' 'test'
// 'test/dir' -> 'test/' 'dir'
// 'test/dir/foo' -> 'test/dir/' 'foo'
// 'test/dir/' -> 'test/' 'dir/'
// '../' -> '' '../'
// 'foo///bar//test///' -> 'foo///bar//' 'test///'

bool SplitFinalElem(const char* path, char** dir, char** final)
{
	size_t pathlen = strlen(path);
	size_t splitat = pathlen;
	while ( splitat && path[splitat-1] == '/' )
		splitat--;
	while ( splitat && path[splitat-1] != '/' )
		splitat--;
	char* retdir = String::Substring(path, 0, splitat);
	if ( !retdir ) { return false; }
	char* retfinal = String::Substring(path, splitat, pathlen - splitat);
	if ( !retfinal ) { delete[] retdir; return false; }
	*dir = retdir;
	*final = retfinal;
	return true;
}

} // namespace Sortix
