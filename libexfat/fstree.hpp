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

#include "exfat.h"

#include <iostream>
#include <map>
#include <unordered_map>
#include <memory>
#include <set>
#include <string>
#include <optional>
#include <cstring>

#include "fsexcept.hpp"

namespace io {
namespace github {
namespace paulyc {

/* The classes below are exported */
/* Don't think we really need this with the namespaces, but reminder to pick one or the other */
#pragma GCC visibility push(default)

class ExFATDirectoryTree
{
public:
    ExFATDirectoryTree(off_t root_directory_offset);
    virtual ~ExFATDirectoryTree();

    void addNode(off_t fde_offset, struct exfat_node_entry &entry);
    void writeRepairJournal(int fd) throw();
    void reconstructLive(int fd) throw();

//private:
    class Node;
    class Directory;
    class RootDirectory;
    class File;

    std::shared_ptr<RootDirectory> _root_directory;
    std::unordered_map<off_t, std::shared_ptr<Node>> _node_offset_map;

    class Node
    {
    public:
        Node(std::string name) : _name(name) {}
        Node(std::string name, std::shared_ptr<Directory> &parent) : _name(name), _parent(parent) {}
        virtual ~Node() {}

        virtual bool loadFromFDE(off_t fde_offset, struct exfat_node_entry &entry) { return false; }

        struct exfat_node_entry &getNodeEntry() { return _entry; }
        bool isFragmented() const { return EXFAT_FLAG_CONTIGUOUS & _entry.efi.flags; }
        size_t getSize() const { return _entry.efi.size.__u64; }
    protected:
        Node(off_t offset, struct exfat_node_entry &entry) : _node_offset(offset), _entry(entry) {}
    //private:
        std::string _name; // utf-8
        off_t _node_offset;
        struct exfat_node_entry _entry;
        std::shared_ptr<Directory> _parent;
    };

    class Directory : public Node
    {
    public:
        Directory(std::string name) : Node(name) {}
        Directory(std::string name, std::shared_ptr<Directory> &parent) : Node(name, parent) {}
        virtual ~Directory() {}

        void addChild(std::shared_ptr<Node> child) noexcept;
    protected:
        Directory(off_t offset, struct exfat_node_entry &entry) : Node(offset, entry) {}
    private:
        std::set<std::shared_ptr<Node>> _children;
    };

    class RootDirectory : public Directory
    {
    public:
        RootDirectory(off_t offset, struct exfat_node_entry &entry) : Directory(offset, entry) {}
    };

    class File : public Node
    {
    public:
        File(std::string name) : Node(name) {}
        File(std::string name, std::shared_ptr<Directory> &parent) : Node(name, parent) {}
        virtual ~File() {}

        void writeToDirectory(struct exfat_dev* dev, off_t fs_offset, std::string dirpath) const;
    private:
    };
};

#pragma GCC visibility pop

}
}
}

#endif /* __cplusplus */

#endif /* fstree_hpp */
