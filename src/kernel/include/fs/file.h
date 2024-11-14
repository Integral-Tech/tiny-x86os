// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef FILE_H
#define FILE_H

#define FILENAME_SIZE 32
#define FILE_TABLE_SIZE 2048

#include "comm/types.h"

typedef enum _file_type_t {
  UNKNOWN_FILE,
  TTY_FILE,
  DIR_FILE,
  NORMAL_FILE
} file_type_t;

struct _fs_t;

typedef struct _file_t {
  char name[FILENAME_SIZE];
  file_type_t type;
  uint32_t size;
  int ref; // number of times the file was opened
  int dev_id, pos, mode;

  struct _fs_t *fs;
  size_t dirent_index, cluster_start, curr_cluster;
} file_t;

file_t *file_alloc();
void file_free(file_t *file);

static inline void file_cleanup(file_t **file) {
  file_free(*file);
}

#define _cleanup_file_ __attribute__((cleanup(file_cleanup)))

void file_table_init();
void file_ref_inc(file_t *file);

#endif
