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

#include <exfat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

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
    spec = argv[optind++];
    fprintf(stderr, "Reconstructing nuked file system on %s.\n", spec);
    dev = exfat_open(spec, EXFAT_MODE_RW);

    do {
        if (dev == NULL) {
            ret = errno;
            fprintf(stderr, "exfat_open(%s) failed: %s\n", spec, strerror(ret));
            break;
        }

        spec = argv[optind];
        logfile = fopen(spec, "r");
        if (logfile == NULL) {
            ret = errno;
            fprintf(stderr, "fopen(%s) failed: %s\n", spec, strerror(ret));
            break;
        }

        /*ret = reconstruct(dev, logfile);
        if (ret != 0) {
            fprintf(stderr, "reconstruct() returned error: %s\n", strerror(ret));
        }*/
    } while (0);

    io::github::paulyc::ExFATFilesystem fs;
    fs.openFilesystem("/dev/disk2", start_offset_bytes, false);
    fs.restoreFilesFromScanLogFile(".dts", "/Users/paulyc/recovery");

    exfat_close(dev);
    fclose(logfile);

    return ret;
}
