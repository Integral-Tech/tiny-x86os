// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DEV_H
#define DEV_H

#include "comm/types.h"
#include <stdarg.h>

#define DEV_NAME_SIZE 32
#define DEV_TABLE_SIZE 128

typedef enum _major_no_t { DEV_UNKNOWN, TTY_DEV, DEV_DISK } major_no_t;

typedef struct _device_t {
  struct _dev_desc_t *desc;
  int mode, minor_no;
  void *data;
  int open_cnt;
} device_t;

typedef struct _dev_desc_t {
  char name[DEV_NAME_SIZE];
  major_no_t major_no;

  int (*open)(device_t *dev);
  int (*close)(const device_t *dev);
  int (*read)(const device_t *dev, uint32_t addr, void *buf, size_t size);
  int (*write)(const device_t *dev, uint32_t addr, const void *buf,
               size_t size);
  int (*control)(const device_t *dev, int cmd, va_list arg_list);
} dev_desc_t;

int dev_open(major_no_t major_no, int minor_no, void *data);
int dev_close(int dev_id);
int dev_read(int dev_id, uint32_t addr, void *buf, size_t size);
int dev_write(int dev_id, uint32_t addr, const void *buf, size_t size);
int dev_control(int dev_id, int cmd, ...);

#endif
