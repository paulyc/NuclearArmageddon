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

#include "exfat++.h"

#include <fstream>
#include <sstream>
#include <string>
#include <memory>

namespace io {
namespace github {
namespace paulyc {

ExFATFilesystem::ExFATFilesystem()
{
    _volume_label = {
        .type = EXFAT_ENTRY_LABEL,                                                 //uint8_t type; /* EXFAT_ENTRY_LABEL */
        .length = 8,                                                               //uint8_t length; /* number of characters */
        .name = { 'E', 'l', 'e', 'm', 'e', 'n', 't', 's', 0, 0, 0, 0, 0, 0, 0 },   //le16_t name[EXFAT_ENAME_MAX]; /* in UTF-16LE */
    };

    _vbr = {
        .sb = {
            //    uint8_t jump[3];                /* 0x00 jmp and nop instructions */
            .jump = {0xEB, 0x76, 0x90},
            //    uint8_t oem_name[8];            /* 0x03 "EXFAT   " */
            .oem_name = {'E', 'X', 'F', 'A', 'T', ' ', ' ', ' '},
            //    uint8_t    __unused1[53];            /* 0x0B always 0 */
            .__unused1 = {0},
            //    le64_t sector_start;            /* 0x40 partition first sector */
            .sector_start = {PARTITION_START_SECTOR},        // 409640
            //    le64_t sector_count;            /* 0x48 partition sectors count */
            .sector_count = {0x1D1B977B7},     // 7813560247
            //    le32_t fat_sector_start;        /* 0x50 FAT first sector */
            .fat_sector_start = {0},
            //    le32_t fat_sector_count;        /* 0x54 FAT sectors count */
            .fat_sector_count = {0},
            //    le32_t cluster_sector_start;    /* 0x58 first cluster sector */
            .cluster_sector_start = {CLUSTER_HEAP_PARTITION_START_SECTOR},
            //    le32_t cluster_count;            /* 0x5C total clusters count */
            .cluster_count = {FAT_CLUSTER_COUNT},
            //    le32_t rootdir_cluster;            /* 0x60 first cluster of the root dir */
            .rootdir_cluster = {0},
            //    le32_t volume_serial;            /* 0x64 volume serial number */
            .volume_serial = {0xdeadbeef},
            //    struct                            /* 0x68 FS version */
            //    {
            //        uint8_t minor;
            //        uint8_t major;
            //    }
            //    version;
            .version = {0, 1},
            //    le16_t volume_state;            /* 0x6A volume state flags */
            .volume_state = {0},
            //    uint8_t sector_bits;            /* 0x6C sector size as (1 << n) */
            .sector_bits = 9,
            //    uint8_t spc_bits;                /* 0x6D sectors per cluster as (1 << n) */
            .spc_bits = 9,
            //    uint8_t fat_count;                /* 0x6E always 1 */
            .fat_count = 1,
            //    uint8_t drive_no;                /* 0x6F always 0x80 */
            .drive_no = 0x80,
            //    uint8_t allocated_percent;        /* 0x70 percentage of allocated space */
            .allocated_percent = 100,
            //    uint8_t __unused2[397];            /* 0x71 always 0 */
            .__unused2 = {0},
            //    le16_t boot_signature;            /* the value of 0xAA55 */
            .boot_signature = {0xAA55},
        },
        .bpb = { {
            .mebs = { {
                {0},        //uint8_t zero[510];
                {0xAA55}    //le16_t boot_signature;
            } },
            .oem_params = { {0}, 0, 0, 0, 0, 0, 0, 0, 0, {0} },
            .zs = { {0} },
            .chksum = {0},
        } },
    };

    _bmp_entry = {
        .type = EXFAT_ENTRY_BITMAP,                    /* EXFAT_ENTRY_BITMAP */
        .bitmap_flags = 0,            /* bit 0: 0 = 1st cluster heap. 1 = 2nd cluster heap. */
        .__unknown1 = {0},
        .start_cluster = {2},
        .size = {(FAT_CLUSTER_COUNT) / 8 + (FAT_CLUSTER_COUNT) % 8} /* in bytes = Ceil (Cluster count / 8 ) */
    };

    _filesystem.dev = nullptr;
    init_fat(&_fat);
    init_cluster_heap(&_fat, &_heap, &_bmp_entry);
    init_upcase_table(&_fat, &_upcase);
    //_directory_tree = std::make_unique<ExFATDirectoryTree>(0);
}

ExFATFilesystem::~ExFATFilesystem() {
    if (_filesystem.dev != nullptr) {
        exfat_close(_filesystem.dev);
    }
}

void ExFATFilesystem::openFilesystem(std::string device_path, off_t filesystem_offset, bool rw) {
    _filesystem.dev = exfat_open(device_path.c_str(), rw ? EXFAT_MODE_RW : EXFAT_MODE_RO);
    if (_filesystem.dev != nullptr) {
        _fs_offset = filesystem_offset;
    } else {
        throw LIBC_EXCEPTION;
    }
}

void ExFATFilesystem::rebuildFromScanLogfile(std::string logfilename, std::function<void(off_t, struct exfat_node_entry&)> fun) {
    std::ifstream logfile(logfilename);
    std::string line;
    size_t line_no = 0;
    while (std::getline(logfile, line)) {
        std::istringstream iss(line);
        iss >> std::hex;
        std::string line_ident, dummy;
        size_t node_offset;
        cluster_t cluster_offset;
        ++line_no;
        _processLine(line, iss, line_no, fun);
    }
}

void ExFATFilesystem::restoreFilesFromScanLogFile(std::string logfilename, std::string output_dir) {
    std::function<void(off_t, struct exfat_node_entry&)> fun =
        [this, &output_dir](off_t fs_offset, struct exfat_node_entry& entry)
    {
        le16_t fname_utf16[EXFAT_NAME_MAX+1];
        le16_t *pfname_utf16 = fname_utf16;
        char fname[EXFAT_UTF8_ENAME_BUFFER_MAX];

        if (entry.fde.attrib.__u16 & EXFAT_ATTRIB_DIR) { // this is a directory, skip
        } else {// going to assume it is a file
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
                std::string name(fname);
                if (name.rfind(".DTS") != std::string::npos || name.rfind(".dts") != std::string::npos) {
                    std::cout << "found file " << name << ", checking contiguity" << std::endl;
                    if (entry.efi.flags & EXFAT_FLAG_CONTIGUOUS) {
                        std::shared_ptr<ExFATDirectoryTree::File> f = std::make_shared<ExFATDirectoryTree::File>(name);
                        std::cout << name << " is contiguous, attempting to write" << std::endl;
                        f->writeToDirectory(_filesystem.dev, _fs_offset, output_dir);
                    } else {
                        std::cerr << name << " is fragmented, giving up" << std::endl;
                    }
                }
            } else {
                std::cerr << "utf16_to_utf8 returned " << res << std::endl;
            }
        }
    };
    this->rebuildFromScanLogfile(logfilename, fun);
}

void ExFATFilesystem::writeRestoreJournal(int fd) const {
}

void ExFATFilesystem::reconstructLive(int fd) {
}

void ExFATFilesystem::_processLine(
    std::string &line,
    std::istringstream &iss,
    size_t line_no,
    std::function<void(off_t, struct exfat_node_entry&)> fun)
{
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
                    _processFileDirectoryEntry(node_offset, fun);
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

void ExFATFilesystem::_processFileDirectoryEntry(
     off_t disk_offset,
     std::function<void(off_t, struct exfat_node_entry&)> fun)
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
    /*
exfat_filesystem_t reconstruct_filesystem_from_scan_logfile(const char *fsdev, const char *logfilename) {
    ExFATFilesystem *fs = new ExFATFilesystem;
    fs->openFilesystem(fsdev, 0, false);
    fs->rebuildFromScanLogfile(logfilename, [](off_t offset, struct exfat_node_entry &entry) {
		// TODO implement
    });
    return fs;
}

void write_fs_reconstruct_journal(exfat_filesystem_t fs, int fd) {
}

void reconstruct_live_fs(exfat_filesystem_t fs, int fd) {
}

void free_filesystem(exfat_filesystem_t fs) {
    delete (ExFATFilesystem*)fs;
}*/

}
}
}
