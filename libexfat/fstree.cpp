//
//  fstree.cpp
//  NuclearHolocaust
//
//  Created by Paul Ciarlo on 1/23/19.
//  Copyright © 2019 Paul Ciarlo. All rights reserved.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//

#include "exfat.h"
#include "fstree.hpp"
#include "fsexcept.hpp"

ExFATDirectoryTree::ExFATDirectoryTree(struct exfat *fs, off_t root_directory_offset) :
    _filesystem(fs)
{

}

ExFATDirectoryTree::~ExFATDirectoryTree() {

}

void ExFATDirectoryTree::addNode(struct exfat_dev *dev, off_t fde_offset) throw() {
    struct exfat_node_entry entry;

    ssize_t rd = exfat_pread(dev, &entry, sizeof(entry), fde_offset);
    if (rd == sizeof(entry)) {
    } else if (rd == 0) {
        return;
    } else if (rd == -1) {
        throw LIBC_EXCEPTION;
    }
}

void ExFATDirectoryTree::writeRepairJournal(int fd) throw() {
    
}

void ExFATDirectoryTree::reconstructLive(int fd) throw() {
    
}
