//
//  bptree.h
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

#ifndef bptree_h
#define bptree_h

#include "exfatfs.h"

#ifdef __cplusplus
extern "C" {
#endif

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
    uint32_t parent_node;                        // parent of this node in the bptree structure
    uint32_t child_nodes[BPTREE_CHILD_NODES];    // offset of child nodes in the bptree structure
}
PACKED;
STATIC_ASSERT(sizeof(struct bptree_node) == 92);

struct bptree
{
    struct bptree_node *heap;
    uint8_t tree_height;        //max height of the
    size_t heap_size;
    uint8_t max_height; //max height of a currently stored node
}
PACKED;

//for start size of 2396745 entries or ~200 MB
#define BPTREE_HEIGHT 8

struct bptree_node * alloc_bptree_heap(uint8_t levels, size_t *heap_size);
void free_bptree_heap(struct bptree_node *bptree_heap);

void get_bptree_entry_heap_offset(uint8_t height, uint32_t level_offset, uint32_t *heap_offset);
void get_bptree_entry_height_and_index(uint32_t heap_offset, uint8_t *height, uint32_t *level_offset);

void init_bptree_heap(struct bptree_node *bptree_heap,
                      struct exfat_entry_meta2 *root_directory,
                      uint64_t root_directory_offset);

void make_bptree(struct bptree *bptree,
                 uint8_t height,
                 struct exfat_entry_meta2 *root_directory,
                 uint64_t root_directory_offset);

void destroy_bptree(struct bptree *bptree);

void insert_bptree_node(struct bptree *bptree,
                        uint32_t *bptree_heap_offset, //out
                        struct exfat_entry_meta2 *entry,
                        uint64_t entry_offset);

#ifdef __cplusplus
}
#endif

#endif /* bptree_h */
