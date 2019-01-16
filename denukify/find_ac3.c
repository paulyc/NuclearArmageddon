//
//  find_ac3.c
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

#include "ac3.h"

/**
 Table 5.6 Sample Rate Codes
 fscod Sampling Rate, kHz
 ‘00’ 48
 ‘01’ 44.1
 ‘10’ 32
 ‘11’ reserved

 Table 5.18 Frame Size Code Table (1 word = 16 bits)
 frmsizecod Nominal Bit Rate fs = 32 kHz
 words/syncframe
 fs = 44.1 kHz
 words/syncframe
 fs = 48 kHz
 words/syncframe
 ‘000000’ (0) 32 kbps 96 69 64
 ‘000001’ (0) 32 kbps 96 70 64
 ‘000010’ (1) 40 kbps 120 87 80
 ‘000011’ (1) 40 kbps 120 88 80
 ‘000100’ (2) 48 kbps 144 104 96
 ‘000101’ (2) 48 kbps 144 105 96
 ‘000110’ (3) 56 kbps 168 121 112
 ‘000111’ (3) 56 kbps 168 122 112
 ‘001000’ (4) 64 kbps 192 139 128
 ‘001001’ (4) 64 kbps 192 140 128
 ‘001010’ (5) 80 kbps 240 174 160
 ‘001011’ (5) 80 kbps 240 175 160
 ‘001100’ (6) 96 kbps 288 208 192
 ‘001101’ (6) 96 kbps 288 209 192
 ‘001110’ (7) 112 kbps 336 243 224
 ‘001111’ (7) 112 kbps 336 244 224
 ‘010000’ (8) 128 kbps 384 278 256
 ‘010001’ (8) 128 kbps 384 279 256
 ‘010010’ (9) 160 kbps 480 348 320
 ‘010011’ (9) 160 kbps 480 349 320
 ‘010100’ (10) 192 kbps 576 417 384
 ‘010101’ (10) 192 kbps 576 418 384
 ‘010110’ (11) 224 kbps 672 487 448
 ‘010111’ (11) 224 kbps 672 488 448
 ‘011000’ (12) 256 kbps 768 557 512
 ‘011001’ (12) 256 kbps 768 558 512
 ‘011010’ (13) 320 kbps 960 696 640
 ‘011011’ (13) 320 kbps 960 697 640
 ‘011100’ (14) 384 kbps 1152 835 768
 ‘011101’ (14) 384 kbps 1152 836 768
 ‘011110’ (15) 448 kbps 1344 975 896
 ‘011111’ (15) 448 kbps 1344 976 896
 ‘100000’ (16) 512 kbps 1536 1114 1024
 ‘100001’ (16) 512 kbps 1536 1115 1024
 ‘100010’ (17) 576 kbps 1728 1253 1152
 ‘100011’ (17) 576 kbps 1728 1254 1152
 ‘100100’ (18) 640 kbps 1920 1393 1280
 ‘100101’ (18) 640 kbps 1920 1394 1280
 */

int getBytesPerSyncframe(uint8_t fs_frmsize_code) {
    uint8_t fscod = (fs_frmsize_code & 0xC0) >> 6;
    uint8_t frmsize_code = fs_frmsize_code & 0x3F;
    int nbr=-999, addFor44_1=-888;
    switch (frmsize_code >> 1) {
        case 0: //32
            nbr=32;addFor44_1=5;break;
        case 1: //40
            nbr=40;addFor44_1=7;break;
        case 2: //48
            nbr=48;addFor44_1=8;break;
        case 3: //56
            nbr=56;addFor44_1=9;break;
        case 4:
            nbr=64;addFor44_1=11;break;
        case 5:
            nbr=80;addFor44_1=14;break;
        case 6:
            nbr=96;addFor44_1=16;break;
        case 7:
            nbr=112;addFor44_1=19;break;
        case 8:
            nbr=128;addFor44_1=22;break;
        case 9:
            nbr=160;addFor44_1=28;break;
        case 10:
            nbr=192;addFor44_1=33;break;
        case 11:
            nbr=224;addFor44_1=39;break;
        case 12:
            nbr=256;addFor44_1=45;break;
        case 13:
            nbr=320;addFor44_1=56;break;
        case 14:
            nbr=384;addFor44_1=67;break;
        case 15:
            nbr=448;addFor44_1=79;break;
        case 16:
            nbr=512;addFor44_1=90;break;
        case 17:
            nbr=576;addFor44_1=101;break;
        case 18:
            nbr=640;addFor44_1=113;break;
        default:
            return -1;
    }

    switch (fscod) {
        case 0:
            return 2 * 2*nbr;
        case 1:
            return 2 * (2*nbr + addFor44_1 + frmsize_code & 1);
        case 2:
            return 2 * 3*nbr;
        default:
            return -1;
    }
}

static struct ac3_syncinfo info;

static const uint16_t crc_tab16[256] =
{
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
    0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
    0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
    0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
    0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
    0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
    0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
    0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
    0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
    0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
    0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
    0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
    0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
    0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
    0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
    0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
    0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
    0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
    0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
    0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040,
};

void find_ac3(const char *devpath)
{
    struct exfat_dev *dev = exfat_open(devpath, EXFAT_MODE_RO);
    size_t cluster = 0x0010b4e8;
    size_t offset = cluster * 0x20000;
    uint8_t maxframe[1920*2];
    //exfat_seek(dev, offset, SEEK_SET);
    info.syncword[0] = 0x0b;
    info.syncword[1] = 0x77;
    uint8_t ch, ch2, *ptr;
    while (true) {
        exfat_seek(dev, offset, SEEK_SET);
        ssize_t rd = exfat_read(dev, maxframe, sizeof(maxframe));
        //ssize_t rd = read(0, &ch, 1);
        if (rd != sizeof(maxframe)) {
            return errno;
        }
        ptr = maxframe;
        while (ptr < maxframe + sizeof(maxframe) - sizeof(struct ac3_syncinfo)) {
            if (ptr[0] == 0x0b && ptr[1] == 0x77) {
                int frmSize = getBytesPerSyncframe(ptr[4]);
                if (frmSize > 2) {
                    exfat_seek(dev, offset, SEEK_SET);
                    exfat_read(dev, maxframe, sizeof(maxframe));
                    int crc_bytes = (frmSize>>2) + (frmSize>>4);
                    uint16_t crc = 0xFFFF;
                    for (uint8_t *crc_ptr = maxframe + 4; crc_ptr < maxframe + crc_bytes; ++crc_ptr) {
                        crc = (crc >> 8) ^ crc_tab16[(uint8_t)(crc ^ *crc_ptr)];
                    }
                    if ((crc & 0xFF) == maxframe[3] && ((crc & 0xFF00) >> 8) == maxframe[2]) {
                        //if (crc == 0x0000) {
                        printf("found good crc [%d] %04x vs %02x%02x at frame offset %zx\n", frmSize, crc, maxframe[2], maxframe[3], offset);
                        offset += frmSize;
                    } else {
                        //printf("bad crc [%d] %04x vs. %02x%02x\n", crc_bytes, crc, maxframe[4], maxframe[3]);
                        offset += sizeof(struct ac3_syncinfo);
                    }
                    break;
                } else {
                    ptr += 2;
                    offset += 2;
                }
            } else {
                ++ptr;
                ++offset;
            }
        }

    }
}
