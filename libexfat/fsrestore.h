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

#ifndef fsrestore_h
#define fsrestore_h

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* exfat_filesystem_t;

/* Public C API */
exfat_filesystem_t reconstruct_filesystem_from_scan_logfile(const char *logfilename);
void write_fs_reconstruct_journal(exfat_filesystem_t fs, int fd);
void reconstruct_live_fs(exfat_filesystem_t fs, int fd);
void free_filesystem(exfat_filesystem_t fs);

#include "recovery.h"

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
    uint64_t _fs_offset;

    std::unique_ptr<ExFATDirectoryTree> _directory_tree;

    struct exfat_file_allocation_table _fat;
    struct exfat_cluster_heap _heap;
    struct exfat_upcase_table _upcase;
    struct exfat_entry_label _volume_label;
    struct exfat_volume_boot_record _vbr;
    struct exfat_entry_bitmap _bmp_entry;

    static constexpr struct exfat_entry_label VOLUME_LABEL =        /* volume label */
    {
        .type = EXFAT_ENTRY_LABEL,                                                 //uint8_t type; /* EXFAT_ENTRY_LABEL */
        .length = 8,                                                               //uint8_t length; /* number of characters */
        .name = { 'E', 'l', 'e', 'm', 'e', 'n', 't', 's', 0, 0, 0, 0, 0, 0, 0 },   //le16_t name[EXFAT_ENAME_MAX]; /* in UTF-16LE */
    };

    static constexpr struct exfat_volume_boot_record VBR = {
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

    static constexpr struct exfat_entry_bitmap BMP_ENTRY =            /* allocated clusters bitmap */
    {
        .type = EXFAT_ENTRY_BITMAP,                    /* EXFAT_ENTRY_BITMAP */
        .bitmap_flags = 0,            /* bit 0: 0 = 1st cluster heap. 1 = 2nd cluster heap. */
        .__unknown1 = {0},
        .start_cluster = {2},
        .size = {(FAT_CLUSTER_COUNT) / 8 + (FAT_CLUSTER_COUNT) % 8} /* in bytes = Ceil (Cluster count / 8 ) */
    };
};

#endif

#endif /* fsrestore_h */
