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

    extfs.h
    Implementation of the extended filesystem.

*******************************************************************************/

#ifndef EXTFS_H
#define EXTFS_H

class Inode;

mode_t HostModeFromExtMode(uint32_t extmode);
uint32_t ExtModeFromHostMode(mode_t hostmode);
void StatInode(Inode* inode, struct stat* st);

#endif
