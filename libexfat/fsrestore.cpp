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

#include <fstream>
#include <sstream>
#include <string>

ExFATFilesystem::ExFATFilesystem() :
	_volume_label(VOLUME_LABEL),
	_vbr(VBR),
	_bmp_entry(BMP_ENTRY)
{
    _filesystem.dev = nullptr;
    init_fat(&_fat);
    init_cluster_heap(&_fat, &_heap, &BMP_ENTRY);
    init_upcase_table(&_fat, &_upcase);
    _directory_tree = std::make_unique<ExFATDirectoryTree>(0);
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

void ExFATFilesystem::rebuildFromScanLogfile(std::string filename) throw() {
    std::ifstream logfile(filename);
    std::string line;
    size_t line_no = 0;
    while (std::getline(logfile, line)) {
        std::istringstream iss(line);
        iss >> std::hex;
        std::string line_ident, dummy;
        size_t node_offset;
        cluster_t cluster_offset;
        ++line_no;
        _processLine(line, iss, line_no);
    }
}

void ExFATFilesystem::restoreFilesFromScanLogFile(std::string filter, std::string output_dir) throw() {
//    exfat
}

void ExFATFilesystem::writeRestoreJournal(int fd) {
}

void ExFATFilesystem::reconstructLive(int fd) {
}

void ExFATFilesystem::_processLine(std::string &line, std::istringstream &iss, size_t line_no) throw() {
    std::string line_ident, dummy;
    size_t node_offset, cluster_offset;

    do {
        if (iss >> line_ident) {
            if (line_ident == "CLUSTER") {
                if (iss >> cluster_offset >> dummy >> node_offset) {
                    std::cerr << "Read up to cluster " << cluster_offset << " disk offset " << node_offset << std::endl;
                } else {
                    break;
                }
            } else if (line_ident == "FDE") {
                if (iss >> node_offset) {
                    _processFileDirectoryEntry(node_offset);
                } else {
                    break;
                }
            } else if (line_ident == "EFI") {
                // just ignore this since we're loading it out of the FDE anyway
            } else if (line_ident == "EFN") {
                // just ignore this since we're loading it out of the FDE anyway
            } else {
                break;
            }
        } else {
            break;
        }
        return;
    } while (0);

    exfat_exception ex;
    ex << "Unknown line format at line number [" << line_no << "]: " << line;
    throw ex;
}

void ExFATFilesystem::_processFileDirectoryEntry(off_t disk_offset) throw() {
    //_processFileDirectoryEntryCb(disk_offset, _directory_tree::addNode);
    _processFileDirectoryEntryCb(disk_offset, [this](off_t fs_offset, struct exfat_node_entry& entry) {
        le16_t fname_utf16[EXFAT_NAME_MAX];
        le16_t *pfname_utf16 = fname_utf16;
        char fname[EXFAT_UTF8_ENAME_BUFFER_MAX];
        char *pfname = fname;

        if (entry.fde.attrib.__u16 & EXFAT_ATTRIB_DIR) { // this is a directory, skip
        } else { // going to assume it is a file
            bool copy = false;
            for (int c = 0; c < entry.fde.continuations - 2; ++c) {
                if (entry.u_continuations[c].ent.type == EXFAT_ENTRY_FILE_NAME) {
                    memcpy(pfname_utf16, entry.u_continuations[c].name.name, EXFAT_ENAME_MAX * sizeof(le16_t));
                    pfname_utf16 += EXFAT_ENAME_MAX;
                }
            }
            pfname_utf16->__u16 = 0;
            int res = utf16_to_utf8(fname, fname_utf16, sizeof(fname), sizeof(le16_t) * EXFAT_NAME_MAX);
            if (res == 0) {
                //check name
                std::string s(fname);
                if (s.rfind(".DTS") != std::string::npos || s.rfind(".dts") != std::string::npos) {
                    //check name
                }
            }
        }
    });
}

void ExFATFilesystem::_processFileDirectoryEntryCb(
    off_t disk_offset,
    std::function<void(off_t, struct exfat_node_entry&)> fun) throw()
{
    struct exfat_node_entry node_entry;
    ssize_t rd = exfat_pread(_filesystem.dev, &node_entry, sizeof(node_entry), disk_offset);
    if (rd == sizeof(struct exfat_node_entry)) {
        fun(disk_offset, node_entry);
    } else if (rd == 0) {
        return;
    } else if (rd == -1) {
        throw LIBC_EXCEPTION;
    }
}

/* C API */
exfat_filesystem_t reconstruct_filesystem_from_scan_logfile(const char *fsdev, const char *logfilename) {
    ExFATFilesystem *fs = new ExFATFilesystem;
    fs->openFilesystem(fsdev, 0, false);
    fs->rebuildFromScanLogfile(logfilename);
    return fs;
}

void write_fs_reconstruct_journal(exfat_filesystem_t fs, int fd) {
}

void reconstruct_live_fs(exfat_filesystem_t fs, int fd) {
}

void free_filesystem(exfat_filesystem_t fs) {
    delete (ExFATFilesystem*)fs;
}
