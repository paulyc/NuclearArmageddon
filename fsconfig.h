#ifndef _FSCONFIG_H_
#define _FSCONFIG_H_

static const size_t sector_size_bytes = 512; // bytes 0x0200
static const size_t sectors_per_cluster = 512; // 0x0200
static const size_t cluster_size_bytes =
sector_size_bytes * sectors_per_cluster; // 0x040000
static const size_t disk_size_bytes = 0x000003a352944000; // 0x0000040000000000; // 4TB
static const size_t cluster_heap_disk_start_sector = 0x8c400;
static const size_t cluster_heap_partition_start_sector = 0x283D8;
static const size_t partition_start_sector = 0x64028;

static const cluster_t start_offset_cluster = 0x00adb000;
static const size_t start_offset_bytes = start_offset_cluster * cluster_size_bytes;
static const size_t start_offset_sector = start_offset_bytes / sector_size_bytes;

#endif /* _FSCONFIG_H_ */
