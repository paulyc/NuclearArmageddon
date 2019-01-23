//
//  fsrestore.hpp
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

#ifndef fsrestore_hpp
#define fsrestore_hpp

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Public C API */
struct ExFATFilesystem;
typedef struct ExFATFilesystem* exfat_filesystem_t;
exfat_filesystem_t reconstruct_filesystem_from_scan_logfile(const char *logfilename);
void write_fs_reconstruct_journal(exfat_filesystem_t fs, int fd);
void reconstruct_live_fs(exfat_filesystem_t fs, int fd);
void free_filesystem(exfat_filesystem_t fs);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

#include <iostream>
#include <map>
#include <unordered_map>
#include <memory>
#include <set>
#include <string>

#include "fstree.hpp"

class ExFATFilesystem
{
public:
    ExFATFilesystem(uint64_t filesystem_offset); // from start of partition or disk
    virtual ~ExFATFilesystem();

    void rebuildFromScanLogfile(std::string filename);
    void writeRestoreJournal(int fd);
    void reconstructLive(int fd);
private:
    std::unique_ptr<ExFATDirectoryTree> _directory_tree;
};

#endif

#endif /* fsrestore_hpp */
