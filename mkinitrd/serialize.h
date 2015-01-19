/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2015.

    This file is part of Sortix.

    Sortix is free software: you can redistribute it and/or modify it under the
    terms of the GNU General Public License as published by the Free Software
    Foundation, either version 3 of the License, or (at your option) any later
    version.

    Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
    details.

    You should have received a copy of the GNU General Public License along
    with Sortix. If not, see <http://www.gnu.org/licenses/>.

    serialize.h
    Import and export binary data structures

*******************************************************************************/

#ifndef SERIALIZE_H
#define SERIALIZE_H

#include <sortix/initrd.h>

void import_initrd_superblock(struct initrd_superblock* obj);
void export_initrd_superblock(struct initrd_superblock* obj);
void import_initrd_inode(struct initrd_inode* obj);
void export_initrd_inode(struct initrd_inode* obj);
void import_initrd_dirent(struct initrd_dirent* obj);
void export_initrd_dirent(struct initrd_dirent* obj);

#endif
