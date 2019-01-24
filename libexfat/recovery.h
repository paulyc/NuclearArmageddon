//
//  recovery.h
//  NuclearHolocaust
//
//  Created by Paul Ciarlo on 1/21/19.
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

#ifndef recovery_h
#define recovery_h

#include <stdio.h>
#include "exfatfs.h"

#define FDE_LOG_FMT "FDE %016zx\n"
#define EFL_LOG_FMT "EFL %016zx %s\n"
#define EFI_LOG_FMT "EFI %016zx\n"
#define EFN_LOG_FMT "EFN %016zx %s\n"

#define SECTOR_SIZE_BYTES ((size_t)512)
#define SECTORS_PER_CLUSTER ((size_t)512)
#define CLUSTER_COUNT ((cluster_t)0xE8DB79)
#define CLUSTER_COUNT_SECTORS (CLUSTER_COUNT * SECTORS_PER_CLUSTER)
#define FAT_CLUSTER_COUNT (CLUSTER_COUNT - 2)
#define CLUSTER_SIZE_BYTES (CLUSTER_COUNT_SECTORS * SECTOR_SIZE_BYTES)
#define DISK_SIZE_BYTES ((size_t)0x000003a352944000)
#define CLUSTER_HEAP_DISK_START_SECTOR ((size_t)0x8c400)
#define CLUSTER_HEAP_PARTITION_START_SECTOR ((size_t)0x283D8)
#define PARTITION_START_SECTOR ((size_t)0x64028)

// rounded to sector boundary
struct exfat_file_allocation_table
{
    cluster_t entries[CLUSTER_COUNT];
    uint8_t padding[SECTOR_SIZE_BYTES - ((CLUSTER_COUNT<<2) % SECTOR_SIZE_BYTES)];
}
PACKED;
STATIC_ASSERT(sizeof(struct exfat_file_allocation_table) == (CLUSTER_COUNT << 2) + SECTOR_SIZE_BYTES - ((CLUSTER_COUNT << 2) % SECTOR_SIZE_BYTES) && (sizeof(struct exfat_file_allocation_table) % SECTOR_SIZE_BYTES) == 0 );

struct exfat_sector
{
    uint8_t data[SECTOR_SIZE_BYTES];
}
PACKED;
STATIC_ASSERT(sizeof(struct exfat_sector) == SECTOR_SIZE_BYTES);

struct exfat_cluster
{
    struct exfat_sector sectors[SECTORS_PER_CLUSTER];
}
PACKED;
STATIC_ASSERT(sizeof(struct exfat_cluster) == SECTOR_SIZE_BYTES*SECTORS_PER_CLUSTER);

//STATIC_ASSERT(sizeof(struct exfat_filesystem) == 1 + 24 + (0xE8DB79+2) );

struct exfat;
struct exfat_dev;

void init_fat(struct exfat_file_allocation_table *fat);
void update_chksum_sector(le32_t *chksum, const uint8_t *const buf, size_t len);
void restore_fat(struct exfat_dev *dev, struct exfat_volume_boot_record *vbr);
struct exfat_node* make_node(void);
void free_node(struct exfat_node *node);
void dump_exfat_entry(union exfat_entries_t *ent, size_t cluster_ofs);
struct exfat_node* try_load_node_from_fde(struct exfat *fs, size_t fde_offset);
void init_filesystem(struct exfat_dev *dev, struct exfat *fs, struct exfat_volume_boot_record *vbr);

#define CLUSTER_HEAP_SIZE (BMAP_SIZE(FAT_CLUSTER_COUNT))
#define CLUSTER_HEAP_SIZE_BYTES (CLUSTER_HEAP_SIZE * sizeof(bitmap_t))

struct exfat_cluster_heap {
    bitmap_t allocation_flags[CLUSTER_HEAP_SIZE];
}
PACKED;
STATIC_ASSERT(sizeof(struct exfat_cluster_heap) == CLUSTER_HEAP_SIZE_BYTES);

int init_cluster_heap(struct exfat_file_allocation_table *fat,
                      struct exfat_cluster_heap *heap,
                      struct exfat_entry_bitmap *bmp_entry);

struct exfat_upcase_table
{
    uint16_t upcase_entries[0xFFFF];
}
PACKED;
STATIC_ASSERT(sizeof(struct exfat_upcase_table) == 0xFFFF * sizeof(uint16_t)); //0x1FFFE

uint32_t upcase_checksum(const uint8_t *const data, size_t data_bytes);
int init_upcase_table(struct exfat_file_allocation_table *fat, struct exfat_upcase_table *tbl);

struct exfat_node_entry * init_directory(struct exfat_file_allocation_table *fat);
void free_directory(struct exfat_node_entry *dir);

int reconstruct(struct exfat_dev *dev, FILE *logfile);

void dump_exfat_entry(union exfat_entries_t *ent, size_t cluster_ofs);

#endif /* recovery_h */
