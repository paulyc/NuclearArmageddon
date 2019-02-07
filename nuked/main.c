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
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

// byte offset 0x458af40000 = cluster 0x115e5b
// 0x115e5b * 0x40000 = 0x45796c0000
// 0x458af40000 - 0x45796c0000 = 0x11880000 (start offset bytes) 0x8C400 (sector)
// cluster 0 = byte offset 0x1181c000 = sector 0x8c0e0

void cluster_search_file_directory_entries(uint8_t *cluster_buf, size_t cluster_size, size_t cluster_ofs_begin) {
    //fprintf(stderr, "cluster_search_file_directory_entries 0x%016zx, 0x%08zx\n", cluster_ofs_begin, cluster_size);
    uint8_t *cluster_ptr = cluster_buf, *cluster_end = cluster_buf + cluster_size;
    union exfat_entries_t *ent;
    struct exfat_entry_meta1 *file_directory_entry = NULL;
    int subcount = 0;
    while (cluster_ptr < cluster_end - sizeof(struct exfat_entry)*2) {
        size_t cluster_ofs = (cluster_ptr - cluster_end) + cluster_ofs_begin;
        // File Directory Entry. starts file entry set.
        //followed by stream extension directory entry (0xC0) and then
        // from 1 to 17 of the file name extension directory entry (0xC1)
        if ((cluster_ofs & 0xFFFFFFF) == 0) {
            fprintf(stderr, "cluster_ofs = %016zx\n", cluster_ofs);
        }
        if (file_directory_entry == NULL) {
            if (*cluster_ptr == EXFAT_ENTRY_FILE &&
                *(cluster_ptr + sizeof(struct exfat_entry)) == EXFAT_ENTRY_FILE_INFO &&
                *(cluster_ptr + 2*sizeof(struct exfat_entry)) == EXFAT_ENTRY_FILE_NAME
                )
            {
                // check the checksum
                file_directory_entry = (struct exfat_entry_meta1 *)cluster_ptr;
                dump_exfat_entry((union exfat_entries_t *)cluster_ptr, cluster_ofs);
                if (file_directory_entry->continuations >= 2 && file_directory_entry->continuations <= 18) { // does not include this entry itself. range 2-18
                    le16_t chksum = exfat_calc_checksum((const struct exfat_entry*)cluster_ptr, file_directory_entry->continuations + 1);
                    if (chksum.__u16 == file_directory_entry->checksum.__u16) {
                        printf(FDE_LOG_FMT, cluster_ofs);
                        subcount = file_directory_entry->continuations;
                        cluster_ptr += sizeof(struct exfat_entry);
                        continue;
                    } else {
						fprintf(stderr, "bad checksum %04x vs. %04x\n", chksum.__u16, file_directory_entry->checksum.__u16);
                    }
                } else {
					fprintf(stderr, "bad number of continuations %d\n", file_directory_entry->continuations);
                }
                file_directory_entry = NULL;
                //cluster_ptr += sizeof(struct exfat_entry);
                //continue;
            } else {
                //cluster_ptr++;// += sizeof(struct exfat_entry);
                //return;
            }
            cluster_ptr++;
        } else {
            if (subcount == 0) {
                file_directory_entry = NULL;
                continue;
            }
            // dont even need to copy just set a pointer
            ent = (union exfat_entries_t*)cluster_ptr;
            switch (ent->ent.type) {
                case EXFAT_ENTRY_FILE:
                {
                    dump_exfat_entry(ent, cluster_ofs);
                    //size_t chksumBytes = (ent->meta1.continuations + 1) * sizeof(exfat_entry);
                    break;
                }
                case EXFAT_ENTRY_BITMAP:
                {
                    dump_exfat_entry(ent, cluster_ofs);
                    break;
                }
                case EXFAT_ENTRY_UPCASE:
                {
                    dump_exfat_entry(ent, cluster_ofs);
                    break;
                }
                case EXFAT_ENTRY_LABEL:
                {
                    char namebuf[EXFAT_ENAME_MAX+1];
                    memset(namebuf, '\0', sizeof(namebuf));
                    for (int i = 0; i < ent->label.length; ++i) {
                        namebuf[i] = (char)(ent->label.name[i].__u16);
                    }
                    printf(EFL_LOG_FMT, cluster_ofs, namebuf);
                    break;
                }
                case EXFAT_ENTRY_FILE_INFO:
                {
                    dump_exfat_entry(ent, cluster_ofs);
                    printf(EFI_LOG_FMT, cluster_ofs);
                    break;
                }
                case EXFAT_ENTRY_FILE_NAME:
                {
                    char namebuf[EXFAT_ENAME_MAX+1];
                    memset(namebuf, '\0', sizeof(namebuf));
                    for (int i = 0; i < EXFAT_ENAME_MAX; ++i) {
                        namebuf[i] = (char)(ent->name.name[i].__u16);
                    }
                    printf(EFN_LOG_FMT, cluster_ofs, namebuf);
                    break;
                }
                case EXFAT_ENTRY_FILE_TAIL:
                {
                    dump_exfat_entry(ent, cluster_ofs);
                    break;
                }
                default:
                {
                    break;
                }
            }
            --subcount;
            cluster_ptr += sizeof(struct exfat_entry);
        }
    }
}

int log_dir_entries(struct exfat_dev *dev) {
    uint8_t cluster_buf[cluster_size_bytes];
    cluster_t c = start_offset_cluster;
    exfat_seek(dev, start_offset_bytes, SEEK_SET);
    for (;;) {
        const size_t cluster_ofs = c * cluster_size_bytes;
        ssize_t rd = exfat_read(dev, cluster_buf, sizeof(cluster_buf));
        if (rd == 0) { // eof
            break;
        } else if (rd == -1) {
            fprintf(stderr, "error reading cluster %08x at offset %016zx: %s\n", c, cluster_ofs, strerror(errno));
            printf("BAD_CLUSTER %08x OFFSET %016zx\n", c, cluster_ofs);
            exfat_seek(dev, cluster_ofs + sizeof(cluster_buf), SEEK_SET);
        } else {
            cluster_search_file_directory_entries(cluster_buf, rd, cluster_ofs);
            if ((c & 0xFFF) == 0) {
                printf("CLUSTER %08x OFFSET %016zx\n", c, cluster_ofs);
            }
        }
        fflush(stdout);
        ++c;
    }
    return 0;
}

static int scan(struct exfat_dev *dev) {
	// run through every cluster, check for directories, write to log

    int ret = log_dir_entries(dev);
    return ret;
}

// 128 KB clusters? = 256 sectors per cluster?
// can recover non-fragmented files from start cluster and length
// but anything fragmented will be difficult to put back together
// Im thinking

static void usage(const char* prog)
{
    fprintf(stderr, "Usage: %s <device>\n", prog);
    fprintf(stderr, "       %s -V\n", prog);
    exit(1);
}

int main(int argc, char* argv[])
{
	int opt, ret;
	const char* options;
	const char* spec = NULL;
    struct exfat_dev *dev;

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
	if (argc - optind != 1)
		usage(argv[0]);
	spec = argv[optind];

	fprintf(stderr, "Reconstructing nuked file system on %s.\n", spec);
    dev = exfat_open(spec, EXFAT_MODE_RO);
    if (dev != NULL) {
        ret = scan(dev);
        if (ret != 0) {
			fprintf(stderr, "reconstruct() returned error: %s\n", strerror(ret));
            return ret;
        }
        //ret = dump
        exfat_close(dev);
    } else {
        ret = errno;
        fprintf(stderr, "open_ro() returned error: %s\n", strerror(ret));
    }

    return 0;
}
