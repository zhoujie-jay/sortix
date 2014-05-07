/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2012, 2013, 2014.

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

    fs/user.h
    User-space filesystem.

*******************************************************************************/

#ifndef SORTIX_FS_USER_H
#define SORTIX_FS_USER_H

#include <sortix/stat.h>

#include <sortix/kernel/refcount.h>
#include <sortix/kernel/vnode.h>

namespace Sortix {
namespace UserFS {

bool Bootstrap(Ref<Inode>* out_root, Ref<Inode>* out_server, const struct stat* rootst);

} // namespace UserFS
} // namespace Sortix

#endif
