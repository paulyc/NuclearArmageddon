cmake_minimum_required(VERSION 3.10)
project(NuclearArmageddon)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

include_directories(libexfat)

add_library(
	exfat++

	SHARED

	libexfat/bptree.c
	libexfat/cluster.c
	libexfat/fsrestore.cpp
	libexfat/fstree.cpp
	libexfat/io.c
	libexfat/log.c
	libexfat/lookup.c
	libexfat/mount.c
	libexfat/node.c
	libexfat/recovery.c
	libexfat/repair.c
	libexfat/time.c
	libexfat/utf.c
	libexfat/utils.c
)

add_executable(denukify denukify/main.cpp)
target_link_libraries(denukify exfat++)

add_executable(dump dump/main.c)
target_link_libraries(dump exfat++)

add_executable(fsck fsck/main.c)
target_link_libraries(fsck exfat++)

#add_executable(fuse fuse/main.c)
#target_link_libraries(fuse exfat++ fuse)

add_executable(label label/main.c)
target_link_libraries(label exfat++)

add_executable(
	mkfs

	mkfs/main.c
	mkfs/vbr.c
	mkfs/cbm.c
	mkfs/rootdir.c
	mkfs/fat.c
	mkfs/uct.c
	mkfs/uctc.c
	mkfs/mkexfat.c
)
target_link_libraries(mkfs exfat++)

add_executable(nuked nuked/main.c)
target_link_libraries(nuked exfat++)
