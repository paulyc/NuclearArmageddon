//
//  fsrestore.cpp
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

#include "fsrestore.h"

ExFATFilesystem::ExFATFilesystem(uint64_t filesystem_offset) :
	_fs_offset(filesystem_offset), // from start of partition or disk
	_volume_label(VOLUME_LABEL),
	_vbr(VBR),
	_bmp_entry(BMP_ENTRY)
{
    init_fat(&_fat);
    init_cluster_heap(&_fat, &_heap, &_bmp_entry);
    init_upcase_table(&_fat, &_upcase);
    _directory_tree = std::make_unique<ExFATDirectoryTree>(0);
}

ExFATFilesystem::~ExFATFilesystem() {

}

void ExFATFilesystem::rebuildFromScanLogfile(std::string filename) {

}

void ExFATFilesystem::writeRestoreJournal(int fd) {

}

void ExFATFilesystem::reconstructLive(int fd) {
    
}
