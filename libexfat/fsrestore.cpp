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

#include "fsrestore.hpp"
#include "fsrestore.h"
#include "fsexcept.hpp"

ExFATFilesystem::ExFATFilesystem() :
	_volume_label(VOLUME_LABEL),
	_vbr(VBR),
	_bmp_entry(BMP_ENTRY)
{
    _filesystem.dev = nullptr;
    init_fat(&_fat);
    init_cluster_heap(&_fat, &_heap, &BMP_ENTRY);
    init_upcase_table(&_fat, &_upcase);
    _directory_tree = std::make_unique<ExFATDirectoryTree>(nullptr, 0);
}

ExFATFilesystem::~ExFATFilesystem() {
    if (_filesystem.dev != nullptr) {
        exfat_close(_filesystem.dev);
    }
}

void ExFATFilesystem::openFilesystem(std::string device_path, off_t filesystem_offset, bool rw) throw() {
    _filesystem.dev = exfat_open(device_path.c_str(), rw ? EXFAT_MODE_RW : EXFAT_MODE_RO);
    if (_filesystem.dev != nullptr) {
        //_fs_offset = filesystem_offset;
    } else {
        throw LIBC_EXCEPTION;
    }
}

void ExFATFilesystem::rebuildFromScanLogfile(std::string filename) {

}

void ExFATFilesystem::writeRestoreJournal(int fd) {

}

void ExFATFilesystem::reconstructLive(int fd) {
    
}
