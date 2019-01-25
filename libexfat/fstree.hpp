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

#include "exfat.h"

#include <iostream>
#include <map>
#include <unordered_map>
#include <memory>
#include <set>
#include <string>

struct exfat;

class ExFATDirectoryTree
{
public:
    ExFATDirectoryTree(struct exfat *fs, off_t root_directory_offset);
    virtual ~ExFATDirectoryTree();

    void addNode(struct exfat_dev *dev, off_t fde_offset) throw();
    void writeRepairJournal(int fd) throw();
    void reconstructLive(int fd) throw();

private:
    class Directory;
    class File;

    class Node
    {
    public:
        Node() {}
        virtual ~Node() {}

        virtual bool loadFromFDE(struct exfat_dev *dev, off_t fde_offset);

        struct exfat_node_entry &getNodeEntry() { return _entry; }
        bool isFragmented() const { return EXFAT_FLAG_CONTIGUOUS & _entry.efi.flags; }
    private:
        off_t _node_offset;
        struct exfat_node_entry _entry;
        std::shared_ptr<Directory> _parent;
        std::string _name; // utf-8
        size_t _size;
    };

    class Directory : public Node
    {
    public:
        Directory() {}
        virtual ~Directory() {}

        void addChild(std::shared_ptr<Node> child);
    private:
        std::set<std::shared_ptr<Node>> _children;
    };

    class File : public Node
    {
    public:
        File() {}
        virtual ~File() {}
    private:
    };

    struct exfat *_filesystem;
    std::shared_ptr<Directory> _root_directory;
    std::unordered_map<off_t, std::shared_ptr<Node>> _node_offset_map;
};

#endif /* fstree_hpp */
