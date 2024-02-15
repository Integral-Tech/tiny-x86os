// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef FATFS_H
#define FATFS_H

#include "ipc/mutex.h"
#include <sys/stat.h>

#define ROOT_ENTRY_SIZE 32

#pragma pack(1)

#define FAT_FILENAME_LEN 11
#define FAT_BASENAME_LEN 8
#define FAT_EXT_LEN 3 // Standard 8.3 format

#define DIRENT_RO 0x1
#define DIRENT_HIDDEN 0x2
#define DIRENT_SYS 0x4
#define DIRENT_VOL_ID 0x8
#define DIRENT_DIR 0x10
#define DIRENT_ARCHIVE 0x20
#define DIRENT_LFN (DIRENT_RO | DIRENT_HIDDEN | DIRENT_SYS | DIRENT_VOL_ID)
// Long File Name

#define DIRENT_NAME_FREE 0xE5
#define DIRENT_NAME_END 0x00

#define CLUSTER_BITS 32
#define CLUSTER_START_NO 2
#define FAT_CLUSTER_INVALID 0xFFF8
#define FAT_CLUSTER_FREE 0

#define is_cluster_valid(cluster)                                              \
  ({                                                                           \
    typeof(cluster) _cluster = (cluster);                                      \
    (_cluster < FAT_CLUSTER_INVALID) && (_cluster >= CLUSTER_START_NO);        \
  })

#define get_cluster_no(dirent)                                                 \
  ({                                                                           \
    typeof(dirent) _dirent = (dirent);                                         \
    (_dirent->first_cluster_hi << 16) | _dirent->first_cluster_lo;             \
  })

#define get_next_dirent(fat, dirent)                                           \
  (dirent_t *)((void *)(dirent) + sizeof(dirent_t))

typedef struct _dbr_t {
  struct {
    uint8_t jmp_code[3], oem_id[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t rsvd_sectors; // Reserved Sectors
    uint8_t fat_num;       // Often this value is 2
    uint16_t root_entries, total_sectors_16;
    uint8_t media_desc;
    uint16_t sectors_per_fat, sectors_per_track, heads_num;
    uint32_t hidden_sectors, total_sectors_32;
  } bpb; // BIOS Parameter Block

  struct {
    uint8_t drive_num, _reserved, sign;
    uint32_t vol_id;
    uint8_t vol_label[FAT_FILENAME_LEN];
    uint8_t sys_id_str[8];
  } ext_boot_rec;
} dbr_t; // DOS Boot Record

typedef struct _dirent_t {
  uint8_t filename[FAT_FILENAME_LEN], file_attr, _reserved;
  uint8_t created_time_in_tenths;
  uint16_t created_time, created_date, last_accessed_date;
  uint16_t first_cluster_hi, last_mod_time, last_mod_date, first_cluster_lo;
  uint32_t file_size; // Size in Bytes
} dirent_t;

#pragma pack()

typedef struct _fat_t {
  uint32_t fat_start, fat_num;
  uint32_t sectors_per_fat, bytes_per_sector, sectors_per_cluster;
  uint32_t root_entries, root_start, data_start, bytes_per_cluster;
  void *fat_buf;
  int curr_sector;
  struct _fs_t *fs;
  mutex_t mutex;
} fat_t;

typedef uint16_t cluster_t;
typedef struct _fs_t fs_t;
typedef struct _DIR DIR;
struct dirent;

int buf_read_sector(fat_t *fat, int sector_no);
int buf_write_sector(fat_t *fat, int sector_no);

int expand_file(file_t *file, size_t incr);

int fatfs_mount(struct _fs_t *fs, int major_no, int minor_no);
int fatfs_unmount(struct _fs_t *fs);
int fatfs_open(struct _fs_t *fs, const char *path, file_t *file);
int fatfs_close(file_t *file);
int fatfs_read(void *buf, size_t size, file_t *file);
int fatfs_write(const void *buf, size_t size, file_t *file);
int fatfs_seek(file_t *file, uint32_t offset, int dir);
int fatfs_stat(file_t *file, struct stat *stat);

void cluster_unlink(fat_t *fat, const dirent_t *prev, const dirent_t *curr);
int fatfs_unlink(fs_t *fs, const char *path);

dirent_t *read_dirent(fat_t *fat, size_t index);
int write_dirent(fat_t *fat, const dirent_t *dirent, size_t index);
void dirent_get_name(const dirent_t *dirent, char *str_buf);
file_type_t dirent_get_type(const dirent_t *dirent);

int fatfs_opendir(fs_t *fs, const char *name, DIR *dir);
int fatfs_readdir(fs_t *fs, DIR *dir, struct dirent *dirent);
int fatfs_closedir(fs_t *fs, DIR *dir);

#endif
