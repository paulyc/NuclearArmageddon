//
//  recovery.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "exfat.h"

void init_fat(struct exfat_file_allocation_table *fat) {
    //const size_t sectors = sizeof(struct exfat_file_allocation_table) / SECTOR_SIZE_BYTES;
    fat->entries[0] = 0x0FFFFFF8; // Media descriptor hard drive
    fat->entries[1] = EXFAT_CLUSTER_END; // Label?
    fat->entries[2] = EXFAT_CLUSTER_END; // To be set to next cluster of the Allocation bitmap
    fat->entries[3] = EXFAT_CLUSTER_END; // Up-Case table
    fat->entries[4] = EXFAT_CLUSTER_END; // Root directory
    for (cluster_t c = 5; c < CLUSTER_COUNT; ++c) {
        fat->entries[c] = EXFAT_CLUSTER_FREE;
    }
}

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

void restore_fat(struct exfat_dev *dev, struct exfat_volume_boot_record *vbr) {
    //exfat_seek(dev, 0, SEEK_SET);
    //exfat_write(dev, &sb, sizeof(struct exfat_super_block)); // 0
    update_chksum_sector(vbr->bpb[0].chksum.chksum, (const uint8_t *const)&vbr->sb, sizeof(struct exfat_super_block));
    for (int i = 0; i < 8; ++i) {
        //exfat_write(dev, &zero_mebs, sizeof(struct mebr_sector)); // 1-8
        //memcpy(vbr.mebs[i], )
        update_chksum_sector(vbr->bpb[0].chksum.chksum, (const uint8_t *const)&vbr->bpb[0].mebs[i], sizeof(struct mebr_sector));
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
    .parent         = NULL, //struct exfat_node* parent;
    .child             = NULL, //struct exfat_node* child;
    .next             = NULL,    //struct exfat_node* next;
    .prev             = NULL,    //struct exfat_node* prev;

    .references     = 0,    //int references;
    .fptr_index     = 0,    //uint32_t fptr_index;
    .fptr_cluster     = 0,    //cluster_t fptr_cluster;
    .entry_offset    = 0,    //off_t entry_offset;
    .start_cluster    = 0,    //cluster_t start_cluster;
    .attrib         = 0,    //uint16_t attrib;
    .continuations    = 0,    //uint8_t continuations;
    .is_contiguous    = 0,    //bool is_contiguous : 1;
    .is_cached        = 0,    //bool is_cached : 1;
    .is_dirty        = 0,    //bool is_dirty : 1;
    .is_unlinked    = 0,    //bool is_unlinked : 1;
    .size            = 0,    //uint64_t size;
    .mtime            = 0,    //time_t mtime
    .atime            = 0,    //time_t atime;
    .name            = {0},    //le16_t name[EXFAT_NAME_MAX + 1];
};

struct exfat_node* make_node() {
    struct exfat_node *node = malloc(sizeof(struct exfat_node));
    memcpy(node, &node_prototype, sizeof(struct exfat_node));
    return node;
}

void free_node(struct exfat_node *node) {
    free(node);
}

struct exfat_node* try_load_node_from_fde(struct exfat *fs, size_t fde_offset) {
// dont think im even using this
#if 0
    struct exfat_node *node = make_node();
    do {
        struct exfat_node_entry node_entry;
        ssize_t rd = exfat_pread(&_filesystem.dev, &node_entry, sizeof(node_entry), fde_offset);
        /*ssize_t res = exfat_seek(fs->dev, fde_offset, SEEK_SET);
        size_t to_read = 3*sizeof(node_entry.efi);
        res = exfat_read(fs->dev, &node_entry, to_read);
        if (res != to_read) { break; }
        const int continuations_left = node_entry.fde.continuations - 3;
        to_read = continuations_left*sizeof(union exfat_entries_t);
        res = exfat_read(fs->dev, node_entry.u_continuations, to_read);*/
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

        //todo

        return node;
    } while(0);
    //fail:
    free_node(node);
#endif
    return NULL;
}

void init_filesystem(struct exfat_dev *dev, struct exfat *fs, struct exfat_volume_boot_record *vbr) {
    fs->dev = dev;
    //fs->upcase =
    fs->sb    = &vbr->sb;

    fs->repair = EXFAT_REPAIR_NO;
}

int init_cluster_heap(struct exfat_file_allocation_table *fat,
                      struct exfat_cluster_heap *heap,
                      const struct exfat_entry_bitmap *const bmp_entry) {
    // mark everything allocated so we don't accidentally overwrite any data
    memset(heap, 0xFF, sizeof(struct exfat_cluster_heap));

    const size_t bmp_size_clusters =
        bmp_entry->size.__u64 / cluster_size_bytes +
        bmp_entry->size.__u64 % cluster_size_bytes;
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
        .flags          = 0,    /* combination of EXFAT_FLAG_xxx */
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
        .name = {{0}},    /* in UTF-16LE */
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

int reconstruct(struct exfat_dev *dev, FILE *logfile) {
    struct exfat fs;
    struct exfat_volume_boot_record vbr; //TODO Needs to be initialized
    struct exfat_file_allocation_table fat;
    struct exfat_cluster_heap heap;
    struct exfat_upcase_table upcase;
    struct bptree bptree;
    struct exfat_entry_meta2 root_directory; // TODO initialize
    size_t root_directory_offset = 0; // TODO initialize
    struct exfat_entry_bitmap bmp_entry =            /* allocated clusters bitmap */
    {
        .type = EXFAT_ENTRY_BITMAP,                    /* EXFAT_ENTRY_BITMAP */
        .bitmap_flags = 0,            /* bit 0: 0 = 1st cluster heap. 1 = 2nd cluster heap. */
        .__unknown1 = {0},
        .start_cluster = {2},
        .size = {(FAT_CLUSTER_COUNT) / 8 + (FAT_CLUSTER_COUNT) % 8} /* in bytes = Ceil (Cluster count / 8 ) */
    };

    init_filesystem(dev, &fs, &vbr);
    //fs->root    = make_node();
    //bptree_heap[0]

    init_fat(&fat);
    int ret = init_cluster_heap(&fat, &heap, &bmp_entry);
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

void dump_exfat_entry(union exfat_entries_t *ent, size_t cluster_ofs) {
    switch (ent->ent.type) {
        case EXFAT_ENTRY_FILE:
        {

            //             struct exfat_entry_meta1            /* file or directory info (part 1) */
            //            {
            //                uint8_t type;                    /* EXFAT_ENTRY_FILE */
            //                uint8_t continuations;
            //                le16_t checksum;
            //                le16_t attrib;                    /* combination of EXFAT_ATTRIB_xxx */
            //                le16_t __unknown1;
            //                le16_t crtime, crdate;            /* creation date and time */
            //                le16_t mtime, mdate;            /* latest modification date and time */
            //                le16_t atime, adate;            /* latest access date and time */
            //                uint8_t crtime_cs;                /* creation time in cs (centiseconds) */
            //                uint8_t mtime_cs;                /* latest modification time in cs */
            //                uint8_t __unknown2[10];
            //            }
            fprintf(stderr, "found EXFAT_ENTRY_FILE: struct exfat_entry_meta1 at %016zx\n", cluster_ofs);
            fprintf(stderr, "type: %02x\n", ent->meta1.type);
            fprintf(stderr, "continuations: %02x\n", ent->meta1.continuations);
            fprintf(stderr, "checksum: %04x\n", ent->meta1.checksum.__u16);
            break;
        }
        case EXFAT_ENTRY_BITMAP:
        {
            fprintf(stderr, "found EXFAT_ENTRY_BITMAP: struct exfat_entry_bitmap at %016zx\n", cluster_ofs);
            fprintf(stderr, "type: %02x\n", ent->bitmap.type);
            break;
        }
        case EXFAT_ENTRY_UPCASE:
        {
            fprintf(stderr, "found EXFAT_ENTRY_UPCASE: struct exfat_entry_upcase at %016zx\n", cluster_ofs);
            fprintf(stderr, "type: %02x\n", ent->upcase.type);
            break;
        }
        case EXFAT_ENTRY_LABEL:
        {
            fprintf(stderr, "found EXFAT_ENTRY_LABEL: struct exfat_entry_label at %016zx\n", cluster_ofs);
            fprintf(stderr, "type: %02x\n", ent->label.type);
            break;
        }
        case EXFAT_ENTRY_FILE_INFO:
        {
            fprintf(stderr, "found EXFAT_ENTRY_FILE_INFO: struct exfat_entry_meta2 at %016zx\n", cluster_ofs);
            fprintf(stderr, "type: %02x\n", ent->meta2.type);
            fprintf(stderr, "name_length: %d\n", ent->meta2.name_length);
            fprintf(stderr, "size: %llu\n", ent->meta2.size.__u64);
            fprintf(stderr, "valid_size: %llu\n", ent->meta2.valid_size.__u64);
            fprintf(stderr, "start_cluster: %08x\n", ent->meta2.start_cluster.__u32);
            fprintf(stderr, "flags: %08x\n", ent->meta2.flags);
            if ((1 << 1) & ent->meta2.flags) {
                fprintf(stderr, "(Contiguous file)\n");
            } else {
                /*if (1 & ent->meta2.flags) {

                 } else {

                 }*/
                fprintf(stderr, "(Fragmented file)\n");
            }
            break;
        }
        case EXFAT_ENTRY_FILE_NAME:
        {
            fprintf(stderr, "found EXFAT_ENTRY_FILE_NAME: struct exfat_entry_name at %016zx\n", cluster_ofs);
            fprintf(stderr, "type: %02x\n", ent->name.type);
            break;
        }
        case EXFAT_ENTRY_FILE_TAIL:
        {
            fprintf(stderr, "found EXFAT_ENTRY_FILE_TAIL at %016zx\n", cluster_ofs);
            break;
        }
        default:
        {
            fprintf(stderr, "found default %02x at %016zx\n", ent->ent.type, cluster_ofs);
            break;
        }
    }
}
