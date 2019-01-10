/*
 main.c (02.09.09)
 exFAT nuclear fallout cleaner-upper

 Free exFAT implementation.
 Copyright (C) 2011-2018  Andrew Nayenko
 Copyright (C) 2018-2019  Paul Ciarlo

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along
 with this program; if not, write to the Free Software Foundation, Inc.,
 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "exfat.h"
#include "exfatfs.h"
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

static const size_t likely_sector_size_bytes = 512; // bytes 0x0200
static const size_t likely_sectors_per_cluster = 256; // 0x0100
static const size_t likely_cluster_size_bytes =
likely_sector_size_bytes * likely_sectors_per_cluster; // 0x020000
static const size_t start_offset_bytes = 0x0000000116FFB000; // from whole disk beginning
static const size_t start_offset_sector =
start_offset_bytes / likely_sector_size_bytes; // from disk beginning 9142232 = 0x008B7FD8
static const size_t start_offset_cluster = start_offset_bytes / likely_cluster_size_bytes; // 0x8B7F;

struct exfat_vbr_t vbr = {
	.sb = {
    //    uint8_t jump[3];                /* 0x00 jmp and nop instructions */
        .jump = { 0xEB, 0x76, 0x90 },
    //    uint8_t oem_name[8];            /* 0x03 "EXFAT   " */
        .oem_name = { 'E', 'X', 'F', 'A', 'T', ' ', ' ', ' ' },
    //    uint8_t    __unused1[53];            /* 0x0B always 0 */
        .__unused1 = { 0 },
    //    le64_t sector_start;            /* 0x40 partition first sector */
        .sector_start = 0x80000, // 524288
    //    le64_t sector_count;            /* 0x48 partition sectors count */
        .sector_count = 0x1D1B7B7DF, // 7813445599
    //    le32_t fat_sector_start;        /* 0x50 FAT first sector */
        .fat_sector_start = 0,
    //    le32_t fat_sector_count;        /* 0x54 FAT sectors count */
        .fat_sector_count = 0,
    //    le32_t cluster_sector_start;    /* 0x58 first cluster sector */
        .cluster_sector_start = 0,
    //    le32_t cluster_count;            /* 0x5C total clusters count */
        .cluster_count = 0,
    //    le32_t rootdir_cluster;            /* 0x60 first cluster of the root dir */
        .rootdir_cluster = 0,
    //    le32_t volume_serial;            /* 0x64 volume serial number */
        .volume_serial = 0xdeadbeef,
    //    struct                            /* 0x68 FS version */
    //    {
    //        uint8_t minor;
    //        uint8_t major;
    //    }
    //    version;
        .version = { 0, 1 },
    //    le16_t volume_state;            /* 0x6A volume state flags */
        .volume_state = 0,
    //    uint8_t sector_bits;            /* 0x6C sector size as (1 << n) */
        .sector_bits = 9,
    //    uint8_t spc_bits;                /* 0x6D sectors per cluster as (1 << n) */
        .spc_bits = 8,
    //    uint8_t fat_count;                /* 0x6E always 1 */
        .fat_count = 1,
    //    uint8_t drive_no;                /* 0x6F always 0x80 */
        .drive_no = 0x80,
    //    uint8_t allocated_percent;        /* 0x70 percentage of allocated space */
        .allocated_percent = 0,
    //    uint8_t __unused2[397];            /* 0x71 always 0 */
        .__unused2 = 0,
    //    le16_t boot_signature;            /* the value of 0xAA55 */
        .boot_signature = 0xAA55,
    },
    .mebs = { { { 0 }, 0xAA55 } },
    .oem_params = { {0}, 0, 0, 0, 0, 0, 0, 0, 0, {0} },
    .zs = { { 0 } },
    .chksum = { 0 },
};

// Sector 10 is reserved, and is not currently defined.
// Sector 11 is a checksum sector, where every 4 byte integer is a 32 bit repeating checksum value
// of the previous 11 sectors. If anyone wanted to tamper with the VBR by changing values in the BPB
// or the boot code, like a boot sector virus infecting the VBR, then the checksum would have to be
// recalculated and sector 11 would need to be updated.
// The last 3 sectors of this 12 sector VBR (sectors 9, 10 and 11) do not contain signatures,
// the signatures are only used for sectors containing boot code and are in the first 9 sectors.

void update_chksum_sector(le32_t *chksum, const uint8_t *const buf, size_t len)
{
    for (size_t i = 0; i < len; ++i) {
        if (i != 106 && i != 107 && i != 112){
            chksum->__u32 = ((chksum->__u32 << 31) | (chksum->__u32 >> 1)) + (uint32_t)buf[i];
        }
    }
}

void restore_fat(struct exfat_dev *dev) {
    //exfat_seek(dev, 0, SEEK_SET);
    //exfat_write(dev, &sb, sizeof(struct exfat_super_block)); // 0
    update_chksum_sector(&vbr.chksum.chksum[0], (const uint8_t *const)&vbr.sb, sizeof(struct exfat_super_block));
    for (int i = 0; i < 8; ++i) {
        //exfat_write(dev, &zero_mebs, sizeof(struct mebr_sector_t)); // 1-8
        //memcpy(vbr.mebs[i], )
        update_chksum_sector(&vbr.chksum.chksum[0], (const uint8_t *const)&vbr.mebs[i], sizeof(struct mebr_sector_t));
    }
    //exfat_write(dev, zero_sector, sizeof(zero_sector)); // 9
	//exfat_write(dev, zero_sector, sizeof(zero_sector)); // 10
    for (size_t i = 1; i < sizeof(vbr.chksum.chksum); ++i) {
        vbr.chksum.chksum[i] = vbr.chksum.chksum[0];
    }
    //exfat_write(dev, &chksum_sector, sizeof(struct chksum_sector_t)); // 11
}

static void usage(const char* prog)
{
    fprintf(stderr, "Usage: %s <device> <logfile>\n", prog);
    fprintf(stderr, "       %s -V\n", prog);
    exit(1);
}

int main(int argc, char* argv[])
{
    int opt, ret;
    const char* options;
    const char* spec = NULL;
    struct exfat_dev *dev;
    FILE *logfile;

    fprintf(stderr, "%s %s\n", argv[0], VERSION);

    while ((opt = getopt(argc, argv, "V")) != -1)
    {
        switch (opt)
        {
            case 'V':
                fprintf(stderr, "Copyright (C) 2011-2018  Andrew Nayenko\n");
                fprintf(stderr, "Copyright (C) 2018-2019  Paul Ciarlo\n");
                return 0;
            default:
                usage(argv[0]);
                break;
        }
    }
    if (argc - optind != 2)
        usage(argv[0]);
    spec = argv[optind];
	// just hardcoding for now? so it cant get screwed up
    //spec = "/dev/disk2s2";
    //fprintf(stderr, "Reconstructing nuked file system on %s.\n", spec);
    dev = exfat_open(spec, EXFAT_MODE_RO);

    if (dev != NULL) {
        spec = argv[optind+1];
        logfile = fopen(spec, "r");
        if (logfile != NULL) {
            /*        ret = reconstruct(dev);
             if (ret != 0) {
             fprintf(stderr, "reconstruct() returned error: %s\n", strerror(ret));
             return ret;
             }
             */        //ret = dump
            fclose(logfile);
        } else {

        }
        exfat_close(dev);
    } else {
        ret = errno;
        fprintf(stderr, "open_ro() returned error: %s\n", strerror(ret));
    }

    return 0;
}
