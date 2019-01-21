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
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>

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

static const size_t sector_size_bytes = SECTOR_SIZE_BYTES; // bytes 0x0200
static const size_t sectors_per_cluster = SECTORS_PER_CLUSTER; // 0x0200
static const size_t cluster_size_bytes = SECTOR_SIZE_BYTES * SECTORS_PER_CLUSTER;
static const size_t disk_size_bytes = DISK_SIZE_BYTES; // 0x0000040000000000; // 4TB
static const size_t cluster_heap_disk_start_sector = CLUSTER_HEAP_DISK_START_SECTOR;
static const size_t cluster_heap_partition_start_sector = CLUSTER_HEAP_PARTITION_START_SECTOR;
static const size_t partition_start_sector = PARTITION_START_SECTOR;
static const size_t cluster_count = CLUSTER_COUNT;

// rounded to sector boundary
struct exfat_file_allocation_table
{
    cluster_t entries[CLUSTER_COUNT];
    uint8_t padding[SECTOR_SIZE_BYTES - ((CLUSTER_COUNT<<2) % SECTOR_SIZE_BYTES)];
}
PACKED;
STATIC_ASSERT(sizeof(struct exfat_file_allocation_table) == (CLUSTER_COUNT << 2) + SECTOR_SIZE_BYTES - ((CLUSTER_COUNT << 2) % SECTOR_SIZE_BYTES) && (sizeof(struct exfat_file_allocation_table) % SECTOR_SIZE_BYTES) == 0 );

struct exfat_sector_t
{
    uint8_t data[SECTOR_SIZE_BYTES];
}
PACKED;
STATIC_ASSERT(sizeof(struct exfat_sector_t) == SECTOR_SIZE_BYTES);

struct exfat_cluster_t
{
    struct exfat_sector_t sectors[SECTORS_PER_CLUSTER];
}
PACKED;
STATIC_ASSERT(sizeof(struct exfat_cluster_t) == SECTOR_SIZE_BYTES*SECTORS_PER_CLUSTER);

//STATIC_ASSERT(sizeof(struct exfat_filesystem) == 1 + 24 + (0xE8DB79+2) );

struct exfat_entry_label volume_label =		/* volume label */
{
	.type = EXFAT_ENTRY_LABEL,                                                 //uint8_t type; /* EXFAT_ENTRY_LABEL */
	.length = 8,                                                               //uint8_t length; /* number of characters */
	.name = { 'E', 'l', 'e', 'm', 'e', 'n', 't', 's', 0, 0, 0, 0, 0, 0, 0 },   //le16_t name[EXFAT_ENAME_MAX]; /* in UTF-16LE */
};

void init_fat(struct exfat_file_allocation_table *fat) {
    const size_t sectors = sizeof(struct exfat_file_allocation_table) / SECTOR_SIZE_BYTES;
    fat->entries[0] = 0x0FFFFFF8; // Media descriptor hard drive
    fat->entries[1] = EXFAT_CLUSTER_END; // Label?
    fat->entries[2] = EXFAT_CLUSTER_END; // To be set to next cluster of the Allocation bitmap
    //fat->entries[3] = EXFAT_CLUSTER_END; // Up-Case table
    //fat->entries[4] = EXFAT_CLUSTER_END; // Root directory
    for (cluster_t c = 3; c < CLUSTER_COUNT; ++c) {
        fat->entries[c] = EXFAT_CLUSTER_FREE;
    }
}

struct exfat_volume_boot_record_t vbr = {
	.sb = {
    //    uint8_t jump[3];                /* 0x00 jmp and nop instructions */
        .jump = { 0xEB, 0x76, 0x90 },
    //    uint8_t oem_name[8];            /* 0x03 "EXFAT   " */
        .oem_name = { 'E', 'X', 'F', 'A', 'T', ' ', ' ', ' ' },
    //    uint8_t    __unused1[53];            /* 0x0B always 0 */
        .__unused1 = { 0 },
    //    le64_t sector_start;            /* 0x40 partition first sector */
        .sector_start = PARTITION_START_SECTOR,		// 409640
    //    le64_t sector_count;            /* 0x48 partition sectors count */
        .sector_count = 0x1D1B977B7, 	// 7813560247
    //    le32_t fat_sector_start;        /* 0x50 FAT first sector */
        .fat_sector_start = 0,
    //    le32_t fat_sector_count;        /* 0x54 FAT sectors count */
        .fat_sector_count = 0,
    //    le32_t cluster_sector_start;    /* 0x58 first cluster sector */
        .cluster_sector_start = CLUSTER_HEAP_PARTITION_START_SECTOR,
    //    le32_t cluster_count;            /* 0x5C total clusters count */
        .cluster_count = FAT_CLUSTER_COUNT,
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
        .spc_bits = 9,
    //    uint8_t fat_count;                /* 0x6E always 1 */
        .fat_count = 1,
    //    uint8_t drive_no;                /* 0x6F always 0x80 */
        .drive_no = 0x80,
    //    uint8_t allocated_percent;        /* 0x70 percentage of allocated space */
        .allocated_percent = 100,
    //    uint8_t __unused2[397];            /* 0x71 always 0 */
        .__unused2 = 0,
    //    le16_t boot_signature;            /* the value of 0xAA55 */
        .boot_signature = 0xAA55,
    },
    .bpb = { {
        .mebs = { { { 0 }, 0xAA55 } },
        .oem_params = { {0}, 0, 0, 0, 0, 0, 0, 0, 0, {0} },
        .zs = { { 0 } },
        .chksum = { 0 },
    } },
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
    update_chksum_sector(vbr.bpb[0].chksum.chksum, (const uint8_t *const)&vbr.sb, sizeof(struct exfat_super_block));
    for (int i = 0; i < 8; ++i) {
        //exfat_write(dev, &zero_mebs, sizeof(struct mebr_sector_t)); // 1-8
        //memcpy(vbr.mebs[i], )
        update_chksum_sector(vbr.bpb[0].chksum.chksum, (const uint8_t *const)&vbr.bpb[0].mebs[i], sizeof(struct mebr_sector_t));
    }
    //exfat_write(dev, zero_sector, sizeof(zero_sector)); // 9
	//exfat_write(dev, zero_sector, sizeof(zero_sector)); // 10
    //for (size_t i = 1; i < sizeof(vbr.bpb[0].chksum.chksum); ++i) {
    //    vbr.bpb[0].chksum.chksum[i] = vbr.bpb[0].chksum.chksum[0];
    //}
    //memcpy(vbr.bpb[1].chksum.chksum, vbr.bpb[0].chksum.chksum, sizeof(vbr.bpb[0].chksum.chksum));
    //exfat_write(dev, &chksum_sector, sizeof(struct chksum_sector_t)); // 11
}

cluster_t find_next_free_cluster(const struct exfat_file_allocation_table *const fat) {
    for (cluster_t c = 0; c < sizeof(fat->entries); ++c) {
        if (fat->entries[c] == EXFAT_CLUSTER_FREE) {
            return c;
        }
    }
    return -1;
}

static struct exfat_node node_prototype = {
    .parent 		= NULL, //struct exfat_node* parent;
    .child 			= NULL, //struct exfat_node* child;
    .next 			= NULL,	//struct exfat_node* next;
    .prev 			= NULL,	//struct exfat_node* prev;

    .references 	= 0,	//int references;
    .fptr_index 	= 0,	//uint32_t fptr_index;
    .fptr_cluster 	= 0,	//cluster_t fptr_cluster;
    .entry_offset	= 0,	//off_t entry_offset;
    .start_cluster	= 0,	//cluster_t start_cluster;
    .attrib 		= 0,	//uint16_t attrib;
    .continuations	= 0,	//uint8_t continuations;
    .is_contiguous	= 0,	//bool is_contiguous : 1;
    .is_cached		= 0,	//bool is_cached : 1;
    .is_dirty		= 0,	//bool is_dirty : 1;
    .is_unlinked	= 0,	//bool is_unlinked : 1;
    .size			= 0,	//uint64_t size;
    .mtime			= 0,	//time_t mtime
    .atime			= 0,	//time_t atime;
    .name			= {0},	//le16_t name[EXFAT_NAME_MAX + 1];
};

struct exfat_node* make_node() {
    struct exfat_node *node = malloc(sizeof(struct exfat_node));
    memcpy(node, &node_prototype, sizeof(struct exfat_node));
    return node;
}

void free_node(struct exfat_node *node) {
    free(node);
}

void dump_exfat_entry(union exfat_entries_t *ent, size_t cluster_ofs);

struct exfat_node* try_load_node_from_fde(struct exfat *fs, size_t fde_offset) {
    struct exfat_node *node = make_node();
    do {
        struct exfat_node_entry node_entry;
        ssize_t res = exfat_seek(fs->dev, fde_offset, SEEK_SET);
        size_t to_read = 3*sizeof(node_entry.efi);
        res = exfat_read(fs->dev, &node_entry, to_read);
        if (res != to_read) { break; }
        const int continuations_left = node_entry.fde.continuations - 3;
        to_read = continuations_left*sizeof(union exfat_entries_t);
        res = exfat_read(fs->dev, node_entry.u_continuations, to_read);
        if (res != to_read) { break; }

        // verify checksum
        //dump_exfat_entry((union exfat_entries_t *)&node_entries, fde_offset);
        const uint8_t continuations = node_entry.fde.continuations;
        if (continuations < 2 || continuations > 18) {
            fprintf(stderr, "bad number of continuations %d\n", continuations);
            break;
        }

        le16_t chksum = exfat_calc_checksum((const struct exfat_entry*)&node_entry, continuations + 1);

        if (chksum.__u16 != node_entry.fde.checksum.__u16) {
            fprintf(stderr, "bad checksum %04x vs. %04x\n", chksum.__u16, node_entry.fde.checksum.__u16);
            break;
        }

		// todo

        return node;
    } while(0);
//fail:
    free_node(node);
    return NULL;
}

void init_filesystem(struct exfat_dev *dev, struct exfat *fs) {
    fs->dev = dev;
    //fs->upcase =
    fs->sb	= &vbr.sb;

    fs->repair = EXFAT_REPAIR_NO;
}

struct exfat_entry_bitmap bmp_entry =            /* allocated clusters bitmap */
{
    .type = EXFAT_ENTRY_BITMAP,                    /* EXFAT_ENTRY_BITMAP */
    .bitmap_flags = 0,            /* bit 0: 0 = 1st cluster heap. 1 = 2nd cluster heap. */
    .__unknown1 = { 0 },
    .start_cluster = 2,
    .size = (FAT_CLUSTER_COUNT) / 8 + (FAT_CLUSTER_COUNT) % 8 /* in bytes = Ceil (Cluster count / 8 ) */
};

#define CLUSTER_HEAP_SIZE (BMAP_SIZE(FAT_CLUSTER_COUNT))
#define CLUSTER_HEAP_SIZE_BYTES (CLUSTER_HEAP_SIZE * sizeof(bitmap_t))

struct exfat_cluster_heap {
    bitmap_t allocation_flags[CLUSTER_HEAP_SIZE];
}
PACKED;
STATIC_ASSERT(sizeof(struct exfat_cluster_heap) == CLUSTER_HEAP_SIZE_BYTES);

int init_cluster_heap(struct exfat_file_allocation_table *fat, struct exfat_cluster_heap *heap) {
    // mark everything allocated so we don't accidentally overwrite any data
    memset(heap, 0xFF, sizeof(struct exfat_cluster_heap));

    const size_t bmp_size_clusters =
        bmp_entry.size.__u64 / cluster_size_bytes +
        bmp_entry.size.__u64 % cluster_size_bytes;
    cluster_t c = 2;
    for (size_t i = 1; i < bmp_size_clusters; ++i) {
        cluster_t next = find_next_free_cluster(fat);
        if (next == -1) {
            return -1;
        } else {
            fat->entries[c] = next;
            c = next;
        }
    }
    fat->entries[c] = EXFAT_CLUSTER_END;

    return 0;
}

struct exfat_upcase_table
{
    uint16_t upcase_entries[0xFFFF];
}
PACKED;
STATIC_ASSERT(sizeof(struct exfat_upcase_table) == 0xFFFF * sizeof(uint16_t)); //0x1FFFE

struct exfat_entry_upcase upcase_entry =    /* upper case translation table */
{
	.type          = EXFAT_ENTRY_UPCASE,   //uint8_t type; /* EXFAT_ENTRY_UPCASE */
	.__unknown1    = {0},                  //uint8_t __unknown1[3];
	.checksum      = {0},                  //le32_t checksum;
	.__unknown2    = {0},                  //.uint8_t __unknown2[12];
	.start_cluster = 0,                    //le32_t start_cluster;
	.size          = sizeof(struct exfat_upcase_table), //le64_t size; /* in bytes */
};

uint32_t upcase_checksum(const uint8_t *const data, size_t data_bytes)
{
    uint32_t chksum = 0;
    for (size_t i = 0; i < data_bytes; ++i) {
        chksum = ((chksum << 31) | (chksum >> 1)) + (uint32_t)data[i];
    }
    return chksum;
}

int init_upcase_table(struct exfat_file_allocation_table *fat, struct exfat_upcase_table *tbl) {
    for (int i = 0; i < sizeof(tbl->upcase_entries); ++i) {
        tbl->upcase_entries[i] = i;
    }
    // ASCII
    for (char ch = 'a'; ch <= 'z'; ++ch) {
        tbl->upcase_entries[ch] = ch ^ 0x20;
    }
    upcase_entry.checksum.__u32 = upcase_checksum((const uint8_t *const)tbl->upcase_entries, sizeof(tbl->upcase_entries));
    cluster_t c = find_next_free_cluster(fat);
    if (c == -1) {
        return -1;
    }
    upcase_entry.start_cluster.__u32 = c;
    fat->entries[c] = EXFAT_CLUSTER_END;
    return 0;
}

struct exfat_node_entry dir_prototype = {
    .fde = {
        .type           = EXFAT_ENTRY_FILE,
        .continuations  = 2,
        .checksum       = {0},
        .attrib         = {0},  /* combination of EXFAT_ATTRIB_xxx */
        .__unknown1     = {0},
        .crtime         = {0},
        .crdate         = {0},  /* creation date and time */
        .mtime          = {0},
        .mdate          = {0},  /* latest modification date and time */
        .atime          = {0},
        .adate          = {0},  /* latest access date and time */
        .crtime_cs      = 0,    /* creation time in cs (centiseconds) */
        .mtime_cs       = 0,    /* latest modification time in cs */
        .atime_cs       = 0,    /* latest access time in cs */
        .__unknown2     = {0},
    },
    .efi = {
        .type           = EXFAT_ENTRY_FILE_INFO,
        .flags          = 0,	/* combination of EXFAT_FLAG_xxx */
        .__unknown1     = 0,
        .name_length    = 0,
        .name_hash      = {0},
        .__unknown2     = {0},
        .valid_size     = {0},  /* in bytes, less or equal to size */
        .__unknown3     = {0},
        .start_cluster  = {0},
        .size           = {0},  /* in bytes */
    },
    .efn = {
        .type = EXFAT_ENTRY_FILE_NAME,
        .__unknown = 0,
        .name = {{0}},	/* in UTF-16LE */
    },
    .u_continuations = {
        {
            .ent = {
                .type = 0,  /* any of EXFAT_ENTRY_xxx */
                .data = {0},
            }
        },
    },
};

struct exfat_node_entry * init_directory(struct exfat_file_allocation_table *fat) {
    struct exfat_node_entry *dir = malloc(sizeof(struct exfat_node_entry));
    memcpy(dir, &dir_prototype, sizeof(struct exfat_node_entry));
    return dir;
}

void free_directory(struct exfat_node_entry *dir) {
    free(dir);
}

#define BPTREE_CHILD_NODES 8

struct bptree_node
{
    struct exfat_entry_meta2 entry; // 32 bytes. file or directory represented by this node
    uint64_t offset;                // offset of this entry on disk (really the exfat_entry_meta1)
    // following are all offsets into the bptree_heap
    uint32_t parent_directory;      // directory containing this file or directory
    uint32_t next_fde;              // next file or directory in the same directory
    uint32_t prev_fde;              // previous file or directory in the same directory
    uint32_t first_directory_entry; // if this is a directory, offset of the bptree_node containing its first entry
    uint32_t parent_node;						// parent of this node in the bptree structure
    uint32_t child_nodes[BPTREE_CHILD_NODES];	// offset of child nodes in the bptree structure
}
PACKED;
STATIC_ASSERT(sizeof(struct bptree_node) == 92);

struct bptree
{
    struct bptree_node *heap;
    uint8_t tree_height;		//max height of the
    size_t heap_size;
    uint8_t max_height; //max height of a currently stored node
}
PACKED;

//for start size of 2396745 entries or ~200 MB
#define BPTREE_HEIGHT 8

struct bptree_node * alloc_bptree_heap(uint8_t levels, size_t *heap_size) {
    *heap_size = 1;
    size_t level_size = 1;
    for (int i = 1; i < levels; ++i) {
        level_size *= BPTREE_CHILD_NODES;
        *heap_size += level_size;
    }
    struct bptree_node *bptree_heap = malloc(sizeof(struct bptree_node) * *heap_size);
    memset(bptree_heap, '\0', sizeof(struct bptree_node) * *heap_size);
    return bptree_heap;
}

void free_bptree_heap(struct bptree_node *bptree_heap) {
    free(bptree_heap);
}

void get_bptree_entry_heap_offset(uint8_t height, uint32_t level_offset, uint32_t *heap_offset) {
    // maybe this can be simplified by doing some simple division or storing the level sizes
    *heap_offset = 0;
    uint32_t level_size = 1;
    while (height--) {
        *heap_offset += level_size;
        level_size *= BPTREE_CHILD_NODES;
    }
    *heap_offset += level_size;
}

void get_bptree_entry_height_and_index(uint32_t heap_offset, uint8_t *height, uint32_t *level_offset) {
    // maybe this can be simplified by doing some simple division or storing the level sizes
    *height = 0;
    uint32_t level_size = 1;
    while (heap_offset > 0) {
        heap_offset -= level_size;
        level_size *= BPTREE_CHILD_NODES;
        ++(*height);
    }
    *level_offset = -heap_offset;
}

void init_bptree_heap(struct bptree_node *bptree_heap,
                      struct exfat_entry_meta2 *root_directory,
                      uint64_t root_directory_offset) {
    memcpy(&bptree_heap[0].entry, root_directory, sizeof(struct exfat_entry_meta2));
    bptree_heap[0].offset = root_directory_offset;
}

void make_bptree(struct bptree *bptree,
                 uint8_t height,
                 struct exfat_entry_meta2 *root_directory,
                 uint64_t root_directory_offset) {
    size_t heap_size;
    bptree->heap = alloc_bptree_heap(height, &heap_size);
    bptree->tree_height = height;
    bptree->heap_size = heap_size;
    bptree->max_height = 0;
    init_bptree_heap(bptree->heap, root_directory, root_directory_offset);
}

void destroy_bptree(struct bptree *bptree) {
    free_bptree_heap(bptree->heap);
}

void insert_bptree_node(struct bptree *bptree,
                        uint32_t *bptree_heap_offset, //out
                        struct exfat_entry_meta2 *entry,
                        uint64_t entry_offset) {
    struct bptree_node *node = bptree->heap;
    uint8_t height = 0;
    uint32_t level_offset = 0;
    uint32_t level_size = 1;
    uint32_t heap_offset;
    //size_t bucket_increment;
    // first, find what level this would go in.
    // if its more than the current maximum level + 1, rebalance the tree
    for (;;) {
        for (int i = 0; i < BPTREE_CHILD_NODES; ++i) {
            if (node->child_nodes[i] == 0) {
                get_bptree_entry_heap_offset(height, level_offset, &heap_offset);
                get_bptree_entry_heap_offset(height + 1, level_offset * BPTREE_CHILD_NODES, bptree_heap_offset);
                node->child_nodes[i] = *bptree_heap_offset;
                node = bptree->heap + *bptree_heap_offset;
                memcpy(&node->entry, entry, sizeof(struct exfat_entry_meta2));
                node->offset = entry_offset;
                node->parent_node = heap_offset;
                return;
            } else if (entry_offset > bptree->heap[node->child_nodes[i]].offset) {
                ++level_offset;
            } else if (entry_offset < bptree->heap[node->child_nodes[i]].offset) {
                get_bptree_entry_heap_offset(height, level_offset, &heap_offset);

                /*if (height >= bptree->height) {

                } else*/
                if (height >= bptree->max_height) {
                    uint8_t heap_offset_height;
                    uint32_t heap_offset_level_offset;

                    // try to pivot the tree, find the next empty node and move the parent and all following nodes over
                    while (bptree->heap[++heap_offset].offset != 0) {
                        get_bptree_entry_height_and_index(heap_offset, &heap_offset_height, &heap_offset_level_offset);
                        if (heap_offset_height >= bptree->tree_height) {
                            assert(heap_offset_level_offset == 0);
                            // TODO : resize the tree. not expected to happen, so not implemented
                            fprintf(stderr, "dynamically resizing the bptree is not currently supported\n");
                            abort();
                        } else if (heap_offset_height > height) {
                            // that level was full, move node down to this position
                            memcpy(bptree->heap + heap_offset, node, sizeof(struct bptree_node));

                            // find and update node's parent's child pointer
                            *bptree_heap_offset = node - bptree->heap;
                            for (int c = 0; c < BPTREE_CHILD_NODES; ++c) {
                                if (bptree->heap[node->parent_node].child_nodes[c] == *bptree_heap_offset) {
                                    bptree->heap[node->parent_node].child_nodes[c] = heap_offset;
                                    break;
                                }
                            }

                            // and update all of node's children's parent pointers
                            for (int c = 0; c < BPTREE_CHILD_NODES; ++c) {
                                if (node->child_nodes[c] != 0) {
                                    bptree->heap[node->child_nodes[c]].parent_node = heap_offset;
                                }
                            }

                            // and store the new node into the old parent position
                            node = bptree->heap + *bptree_heap_offset;
                            memcpy(&node->entry, entry, sizeof(struct exfat_entry_meta2));
                            node->offset = entry_offset;
                            return;
                        }
                    }


                    if (heap_offset_height == height) {
						// move the parent and all following nodes over
                        memmove(bptree->heap + heap_offset, node, (node - bptree->heap) * sizeof(struct bptree_node));
                    } else {
                        //otherwise nothing we can do, its full, increment the max height
                        ++bptree->max_height;
                    }
                }
                ++height;
                level_offset = level_offset * BPTREE_CHILD_NODES;
                level_size *= BPTREE_CHILD_NODES;
                node = bptree->heap + node->child_nodes[i];
                break;
            } else {
                //equal?, not supposed to happen since each entry in the tree is unique
            }
        }
    }
}

int reconstruct(struct exfat_dev *dev, FILE *logfile) {
    struct exfat fs;
    struct exfat_file_allocation_table fat;
    struct exfat_cluster_heap heap;
    struct exfat_upcase_table upcase;
    struct bptree bptree;
    struct exfat_entry_meta2 root_directory; // TODO initialize
    size_t root_directory_offset = 0; // TODO initialize

    init_filesystem(dev, &fs);
    //fs->root	= make_node();
    //bptree_heap[0]

    init_fat(&fat);
    int ret = init_cluster_heap(&fat, &heap);
    if (ret != 0) {
        return ret;

    }
    init_upcase_table(&fat, &upcase);

    make_bptree(&bptree, BPTREE_HEIGHT, &root_directory, root_directory_offset);

    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    size_t offset;
    size_t scanf_str_sz = 1024;
    char *scanf_str = malloc(scanf_str_sz);

    while ((read = getline(&line, &len, logfile)) != -1) {
        //printf("Retrieved line of length %zu:\n", read);
        //printf("%s", line);
        while (read > scanf_str_sz) {
            scanf_str_sz <<= 1;
            scanf_str = realloc(scanf_str, scanf_str_sz);
        }

        if (sscanf(line, FDE_LOG_FMT, &offset) != 0) {
			// insert
        } else if (sscanf(line, EFL_LOG_FMT, &offset, scanf_str)) {
            //EFL
        } else if (sscanf(line, EFI_LOG_FMT, &offset)) {
            //EFL
        } else if (sscanf(line, EFN_LOG_FMT, &offset, scanf_str)) {
            //EFN
        }

        // parse each line
    }

    free(scanf_str);

    // TODO write FAT to disk or log
    //free_node(fs->root);
    destroy_bptree(&bptree);
    return 0;
}

static void usage(const char* prog)
{
    fprintf(stderr, "Usage: %s <device> <logfile>\n", prog);
    fprintf(stderr, "       %s -V\n", prog);
    exit(1);
}

int main(int argc, char* argv[])
{
    int opt, ret = 0;
    const char* options;
    const char* spec = NULL;
    struct exfat_dev *dev = NULL;
    FILE *logfile = NULL;

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
    fprintf(stderr, "Reconstructing nuked file system on %s.\n", spec);
    dev = exfat_open(spec, EXFAT_MODE_RW);

    do {
        if (dev == NULL) {
            ret = errno;
            fprintf(stderr, "exfat_open(%s) failed: %s\n", spec, strerror(ret));
            break;
        }

        spec = argv[optind+1];
        logfile = fopen(spec, "r");
        if (logfile == NULL) {
            ret = errno;
            fprintf(stderr, "fopen(%s) failed: %s\n", spec, strerror(ret));
            break;
        }

        ret = reconstruct(dev, logfile);
        if (ret != 0) {
            fprintf(stderr, "reconstruct() returned error: %s\n", strerror(ret));
        }
    } while (0);

    exfat_close(dev);
    fclose(logfile);

    return ret;
}
