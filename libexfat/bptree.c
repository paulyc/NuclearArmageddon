//
//  bptree.c
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

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "bptree.h"

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

// TODO refactor these gnarly nested blocks
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
