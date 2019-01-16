//
//  ac3.h
//  denukify
//
//  Created by Paul Ciarlo on 1/15/19.
//  Copyright © 2019 Paul Ciarlo <paul.ciarlo@gmail.com>
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

#ifndef find_ac3_h
#define find_ac3_h

#include <stdio.h>
#include "../libexfat/compiler.h"

struct ac3_syncinfo
{
    uint8_t syncword[2]; /* 0x0b 0x77 */
    union {
        uint16_t crcword; /* This 16 bit-CRC applies to the first 5/8 of the syncframe.
                           not including the sync word
                           5/8_framesize = (int) (framesize>>1) + (int) (framesize>>3) ;
                           5/8_framesize = (int) (bytes>>2) + (int) (bytes>>4) ;
                           Transmission of the CRC, like other numerical values,
                           is most significant bit first. */
        uint8_t crcbytes[2];
    } crc;
    uint8_t fs_frmsize_code;
}
PACKED;
STATIC_ASSERT(sizeof(struct ac3_syncinfo) == 5);

struct ac3_bitstreaminfo
{
    uint8_t bs_id_mod;
}
PACKED;
//STATIC_ASSERT(sizeof(struct ac3_bitstreaminfo) == 40);

struct ac3_audioblk
{
}
PACKED;
//STATIC_ASSERT(sizeof(struct ac3_audioblk) == 0);

struct ac3_auxdata
{
    uint16_t syncword;
    uint16_t crc1;
    uint8_t fs_frmsize_code;
}
PACKED;
STATIC_ASSERT(sizeof(struct ac3_auxdata) == 5);

struct ac3_errorcheck
{
    uint16_t syncword;
    uint16_t crc1;
    uint8_t fs_frmsize_code;
}
PACKED;
STATIC_ASSERT(sizeof(struct ac3_errorcheck) == 5);

struct ac3_syncframe
{
    struct ac3_syncinfo syncinfo;
    struct ac3_bitstreaminfo bitstreaminfo;
    struct ac3_audioblk audioblks[6];
    struct ac3_auxdata auxdata;
    struct ac3_errorcheck errcheck;
}
PACKED;
//STATIC_ASSERT(sizeof(struct ac3_errorcheck) == 40);

#endif /* find_ac3_h */
