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

#include "exfat++.h"

#include <fstream>
#include <iomanip>
#include <cstdio>

namespace io {
namespace github {
namespace paulyc {

ExFATDirectoryTree::ExFATDirectoryTree(off_t root_directory_offset)
{
    struct exfat_node_entry entry;
    _root_directory = std::make_shared<RootDirectory>(root_directory_offset, entry);
}

ExFATDirectoryTree::~ExFATDirectoryTree() {

}

void ExFATDirectoryTree::addNode(off_t fde_offset, struct exfat_node_entry &entry) {
    // verify checksum
    const uint8_t continuations = entry.fde.continuations;
    if (continuations < 2 || continuations > 18) {
        exfat_exception except;
        except << "bad number of continuations " << continuations;
        throw except;
    }

    le16_t chksum = exfat_calc_checksum((const struct exfat_entry*)&entry, continuations + 1);

    if (chksum.__u16 != entry.fde.checksum.__u16) {
        exfat_exception except;
        except << "bad checksum " << std::hex << std::setw(4) << chksum.__u16 << " vs. " << entry.fde.checksum.__u16;
        throw except;
    }

    if (entry.fde.attrib.__u16 & EXFAT_ATTRIB_DIR) { // this is a directory
        //a
    } else { // going to assume it is a file
    }
}

void ExFATDirectoryTree::writeRepairJournal(int fd) throw() {
    
}

void ExFATDirectoryTree::reconstructLive(int fd) throw() {
    
}

#define DEFAULT_CLUSTER_SIZE (512*512)
void ExFATDirectoryTree::File::writeToDirectory(struct exfat_dev* dev, off_t fs_offset, std::string dirpath) const {
    uint8_t buffer[DEFAULT_CLUSTER_SIZE];
    const cluster_t cluster_ofs = this->_entry.efi.start_cluster.__u32;
    off_t disk_offset = cluster_ofs * DEFAULT_CLUSTER_SIZE + fs_offset; // todo fix dyanmic cluster size
    size_t bytes_remaining = this->_entry.efi.size.__u64;
    size_t bytes_written = 0;

    FILE *outfile = fopen((dirpath + "/" + this->_name).c_str(), "wb");

    while (bytes_remaining > 0) {
        ssize_t read = exfat_pread(dev, buffer, std::min(bytes_remaining, sizeof(buffer)), disk_offset);
        if (read < 0) { //error
            throw LIBC_EXCEPTION;
        } else if (read == 0) { //eof
            std::cerr << "exfat_pread returned eof while bytes_remaining is " << bytes_remaining << std::endl;
            break;
        }

        size_t written = fwrite(buffer, sizeof(uint8_t), read, outfile);
        if (written != read) {
            throw LIBC_EXCEPTION;
        }

        bytes_remaining -= read;
        bytes_written += read;
        disk_offset += read;
    }

    fclose(outfile);
}

}
}
}
