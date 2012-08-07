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

    sortix/mount.h
    Constants related to mounting and binding.

*******************************************************************************/

#ifndef SORTIX_INCLUDE_MOUNT_H
#define SORTIX_INCLUDE_MOUNT_H

#include <features.h>

__BEGIN_DECLS

#define MREPL   (0<<0) /* Replace binding. */
#define MBEFORE (1<<0) /* Add to start of directory union. */
#define MAFTER  (2<<0) /* Add to end of directory union. */
#define MCREATE (1<<2) /* Create files here, otherwise try next in union. */

__END_DECLS

#endif
