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

#if 0

#include <exfat++.h>
#include <iostream>
#include <string>
#include <exception>

static void usage(const char* prog)
{
    fprintf(stderr, "Usage: %s <device> <logfile> <outdir>\n", prog);
    fprintf(stderr, "       %s -V\n", prog);
    exit(1);
}

int main(int argc, char* argv[])
{
    int opt, ret = 0;
    std::string devspec, logspec, dirspec;

    fprintf(stderr, "%s %s\n", argv[0], VERSION);

    if (argc == 2 && std::string(argv[1]) == std::string("-V")) {
        fprintf(stderr, "Copyright (C) 2011-2018  Andrew Nayenko\n");
        fprintf(stderr, "Copyright (C) 2018-2019  Paul Ciarlo\n");
        return 0;
    } else if (argc != 4) {
        usage(argv[0]);
    }

    devspec = argv[1];
    logspec = argv[2];
    dirspec = argv[3];

    std::cerr << "Reconstructing nuked file system on " << devspec << " from logfile " << logspec << " to directory " << dirspec << std::endl;;

    try {
        io::github::paulyc::ExFATFilesystem fs;
        fs.openFilesystem(devspec, start_offset_bytes, false);
        fs.restoreFilesFromScanLogFile(logspec, dirspec);
    } catch (const std::exception &e) {
        fprintf(stderr, "Got exception restoring files: %s\n", e.what());
    }

    return ret;
}

#else

#include <iostream>
#include <string>

#include <exfat++.h>

static void usage(const std::string &prog)
{
    std::cerr << "Usage: " << prog << " <device> <logfile> <outdir>" << std::endl;
    std::cerr << '\t' << prog << " -V" << std::endl;
}

int main(int argc, char **argv) {
    std::string appspec, devspec, logspec, dirspec;

    appspec = argv[0];
    std::cerr << appspec << ' ' << VERSION << std::endl;
    if (argc == 2 && std::string(argv[1]) == std::string("-V")) {
        fprintf(stderr, "Copyright (C) 2011-2018  Andrew Nayenko\n");
        fprintf(stderr, "Copyright (C) 2018-2019  Paul Ciarlo\n");
        return 0;
    } else if (argc != 4) {
        usage(argv[0]);
        return -1;
    }

    devspec = argv[1];
    logspec = argv[2];
    dirspec = argv[3];

    try {
        io::github::paulyc::ExFATFilesystem fs;
        fs.openFilesystem(devspec, start_offset_bytes, false);
        fs.restoreFilesFromScanLogFile(logspec, dirspec);
    } catch (const std::exception &e) {
        std::cerr << "Got exception restoring files: " << e.what() << std::endl;
        return -2;
    }

    return 0;
}

#endif
