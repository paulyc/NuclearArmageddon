//
//  fsrestore.h
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

#ifndef fsrestore_h
#define fsrestore_h

#ifdef __cplusplus
extern "C" {
#endif

typedef void* exfat_filesystem_t;

/* Public C API */
exfat_filesystem_t reconstruct_filesystem_from_scan_logfile(const char *fsdev, const char *logfilename);
void write_fs_reconstruct_journal(exfat_filesystem_t fs, int fd);
void reconstruct_live_fs(exfat_filesystem_t fs, int fd);
void free_filesystem(exfat_filesystem_t fs);

#ifdef __cplusplus
}
#endif

#endif /* fsrestore_h */
