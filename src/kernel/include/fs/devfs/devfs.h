// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DEV_FS_H
#define DEV_FS_H

#include "dev/dev.h"
#include "fs/fs.h"

typedef struct _devfs_type_t {
  const char *name;
  major_no_t dev_type;
  file_type_t file_type;
} devfs_type_t;

int devfs_mount(fs_t *fs, int major_no, int minor_no);
int devfs_unmount(fs_t *fs);
int devfs_open(fs_t *fs, const char *path, file_t *file);
int devfs_close(file_t *file);
int devfs_read(void *buf, size_t size, file_t *file);
int devfs_write(const void *buf, size_t size, file_t *file);
int devfs_seek(file_t *file, uint32_t offset, int dir);
int devfs_stat(file_t *file, struct stat *stat);
int devfs_ioctl(file_t *file, int cmd, void *arg0, void *arg1);

#endif
