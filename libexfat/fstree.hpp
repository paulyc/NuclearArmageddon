//
//  fstree.hpp
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

#ifndef fstree_hpp
#define fstree_hpp

#ifdef __cplusplus

#include "exfatfs.h"

#include <stdio.h>

#include <iostream>
#include <map>
#include <unordered_map>
#include <memory>
#include <set>
#include <string>

class ExFATDirectoryTree
{
public:
    ExFATDirectoryTree(uint64_t root_directory_offset);
    virtual ~ExFATDirectoryTree();

    void addNode(uint64_t fde_offset);
    void writeRepairJournal(int fd);
    void reconstructLive(int fd);

private:
    struct TreeNode
    {
        TreeNode() : node_offset(0) {}
        virtual ~TreeNode() {}

        uint64_t node_offset;
        struct exfat_node_entry entry;
        std::shared_ptr<TreeNode> parent;
        std::string name; // utf-8

        bool is_fragmented() const { return EXFAT_FLAG_CONTIGUOUS & entry.efi.flags; }
    };
    struct DirectoryNode : public TreeNode
    {
        std::set<std::shared_ptr<TreeNode>> children;
    };
    struct FileNode : public TreeNode
    {
    };

    DirectoryNode _root_directory;
    std::unordered_map<uint64_t, std::shared_ptr<TreeNode>> _node_offset_map;
};

#endif /* __cplusplus */

#endif /* fstree_hpp */
