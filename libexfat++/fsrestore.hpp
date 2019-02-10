//
//  fsrestore.hpp
//  NuclearHolocaust
//
//  Created by Paul Ciarlo on 1/25/19.
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

#ifndef fsrestore_hpp
#define fsrestore_hpp

#include <iostream>
#include <map>
#include <unordered_map>
#include <memory>
#include <set>
#include <string>
#include <exception>
#include <functional>
#include <cerrno>
#include <cstring>

/* The classes below are exported */
#pragma GCC visibility push(default)

namespace io {
namespace github {
namespace paulyc {

class ExFATFilesystem
{
public:
    ExFATFilesystem();
    virtual ~ExFATFilesystem();

    void openFilesystem(std::string device_path, off_t filesystem_offset, bool rw); // from start of partition or disk
    void rebuildFromScanLogfile(std::string logfilename, std::function<void(off_t, struct exfat_node_entry&)> fun);
    void writeRestoreJournal(int fd) const;
    void reconstructLive(int fd);
    void restoreFilesFromScanLogFile(std::string logfilename, std::string output_dir);

private:
    void _processLine(std::string &line, std::istringstream &iss, size_t line_no, std::function<void(off_t, struct exfat_node_entry&)> fun);
    void _processFileDirectoryEntry(off_t disk_offset, std::function<void(off_t, struct exfat_node_entry&)> fun);

    std::string _device_path;
    off_t _fs_offset;

    //std::unique_ptr<ExFATDirectoryTree> _directory_tree;

    struct exfat _filesystem;
    struct exfat_file_allocation_table _fat;
    struct exfat_cluster_heap _heap;
    struct exfat_upcase_table _upcase;
    struct exfat_entry_label _volume_label;
    struct exfat_volume_boot_record _vbr;
    struct exfat_entry_bitmap _bmp_entry;
};

}
}
}

#pragma GCC visibility pop

#endif /* fsrestore_hpp */
