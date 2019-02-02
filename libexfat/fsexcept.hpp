//
//  fsexcept.hpp
//  NuclearHolocaust
//
//  Created by Paul Ciarlo on 1/25/19.
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

#ifndef fsexcept_hpp
#define fsexcept_hpp

#include "compiler.h"

#include <exception>
#include <string>
#include <cerrno>
#include <cstring>
#include <sstream>

class libc_exception : public std::exception
{
public:
    libc_exception(int e) : _errno(e) {}
    libc_exception(const char *prefix, int e) : _prefix("[" + std::string(prefix) + "] "), _errno(errno) {}
    virtual const char* what() const noexcept { return (_prefix + strerror(_errno)).c_str(); }
private:
    std::string _prefix;
    int _errno;
};

class exfat_exception : public std::exception
{
    public:
        exfat_exception() {}
        exfat_exception(const exfat_exception &ex) { _oss << ex._oss.str(); }
        virtual ~exfat_exception() {}

        template <typename T>
        exfat_exception &operator<<(T output) { _oss << output; return *this; }

        virtual const char* what() const noexcept { return _oss.str().c_str(); }
    private:
        std::ostringstream _oss;
};

#define LIBC_EXCEPTION libc_exception(FILELINE, errno)

#endif /* fsexcept_hpp */
