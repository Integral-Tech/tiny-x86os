#include "fs/fatfs/fatfs.h"
#include "core/memory.h"
#include "dev/dev.h"
#include "fs/fs.h"
#include "sys/_default_fcntl.h"
#include "tools/klib.h"
#include "tools/log.h"

fs_api_t fatfs_api = {.mount = fatfs_mount,
                      .unmount = fatfs_unmount,
                      .open = fatfs_open,
                      .close = fatfs_close,
                      .read = fatfs_read,
                      .write = fatfs_write,
                      .seek = fatfs_seek,
                      .stat = fatfs_stat,
                      .opendir = fatfs_opendir,
                      .readdir = fatfs_readdir,
                      .closedir = fatfs_closedir,
                      .unlink = fatfs_unlink};

int fatfs_mount(fs_t *fs, int major_no, int minor_no) {
  const int dev_id = dev_open(major_no, minor_no, NULL);
  if (dev_id < 0) {
    log_printf("Failed to open disk: major number = %x, minor number = %x",
               major_no, minor_no);
    return -1;
  }

  dbr_t *dbr = (dbr_t *)memory_alloc_page();
  if (!dbr) {
    log_printf("Failed to allocate memory for the DBR!");
    goto mount_failed;
  }

  if (dev_read(dev_id, 0, dbr, 1) < 1) {
    log_printf("Failed to read DBR!");
    goto mount_failed;
  }

  fat_t *fat = &fs->fat_data;
  // DBR - FAT1 - FAT2 - root dir - file data
  fat->fat_start = dbr->bpb.rsvd_sectors;
  fat->fat_num = dbr->bpb.fat_num;
  fat->sectors_per_fat = dbr->bpb.sectors_per_fat;
  fat->bytes_per_sector = dbr->bpb.bytes_per_sector;
  fat->sectors_per_cluster = dbr->bpb.sectors_per_cluster;
  fat->root_start = fat->fat_start + fat->sectors_per_fat * fat->fat_num;
  fat->root_entries = dbr->bpb.root_entries;
  fat->data_start =
      fat->root_start + fat->root_entries * ROOT_ENTRY_SIZE / SECTOR_SIZE;
  fat->bytes_per_cluster = fat->sectors_per_cluster * dbr->bpb.bytes_per_sector;
  fat->curr_sector = -1;
  fat->fat_buf = dbr;
  fat->fs = fs;
  mutex_init(&fat->mutex);
  fs->mutex = &fat->mutex;

  if (fat->fat_num != 2)
    log_printf("Warning: The number of FAT is not 2 (major number = %x, minor "
               "number = %x)",
               major_no, minor_no);

  if (kernel_strncmp((char *)dbr->ext_boot_rec.sys_id_str, "FAT16", 5)) {
    log_printf("Invalid FAT16 file system!");
    goto mount_failed;
  }

  fs->type = FAT16;
  fs->data = &fs->fat_data;
  fs->dev_id = dev_id;
  return 0;

mount_failed:
  if (dbr)
    memory_free_page((uint32_t)dbr);

  dev_close(dev_id);
  return -1;
}

int fatfs_unmount(fs_t *fs) {
  fat_t *fat = fs->data;
  dev_close(fs->dev_id);
  memory_free_page((uint32_t)fat->fat_buf);
  return 0;
}

static void read_from_dirent(file_t *file, const dirent_t *dirent,
                             size_t index) {
  file->size = dirent->file_size;
  file->type = dirent_get_type(dirent);
  file->pos = 0;
  file->dirent_index = index;
  file->cluster_start = get_cluster_no(dirent);
  file->curr_cluster = file->cluster_start;
}

static int get_next_cluster(fat_t *fat, cluster_t curr) {
  if (!is_cluster_valid(curr))
    return FAT_CLUSTER_INVALID;

  const size_t sector_no = curr * sizeof(cluster_t) / fat->bytes_per_sector;
  const size_t sector_offset = curr * sizeof(cluster_t) % fat->bytes_per_sector;

  if (sector_no > fat->sectors_per_fat) {
    log_printf("Invalid cluster number: %d", curr);
    return FAT_CLUSTER_INVALID;
  }

  if (buf_read_sector(fat, fat->fat_start + sector_no) < 0)
    return FAT_CLUSTER_INVALID;

  return *(cluster_t *)(fat->fat_buf + sector_offset);
}

static int set_next_cluster(fat_t *fat, cluster_t curr, cluster_t next) {
  if (!is_cluster_valid(curr))
    return -1;

  size_t sector_no = curr * sizeof(cluster_t) / fat->bytes_per_sector;
  const size_t sector_offset = curr * sizeof(cluster_t) % fat->bytes_per_sector;

  if (sector_no > fat->sectors_per_fat) {
    log_printf("Invalid cluster number: %d", curr);
    return -1;
  }

  if (buf_read_sector(fat, fat->fat_start + sector_no) < 0)
    return -1;

  *(cluster_t *)(fat->fat_buf + sector_offset) = next;
  for (size_t i = 0; i < fat->fat_num; i++, sector_no += fat->sectors_per_fat) {
    if (buf_write_sector(fat, fat->fat_start + sector_no) < 0) {
      log_printf("Failed to write cluster in FAT%d!", i);
      return -1;
    }
  }

  return 0;
}

static int move_file_pos(file_t *file, fat_t *fat, uint32_t bytes,
                         _Bool expand) {
  if (file->pos % fat->bytes_per_cluster + bytes >= fat->bytes_per_cluster) {
    cluster_t next_cluster = get_next_cluster(fat, file->curr_cluster);
    if (next_cluster == FAT_CLUSTER_INVALID && expand) {
      if (expand_file(file, fat->bytes_per_cluster) < 0)
        return -1;

      next_cluster = get_next_cluster(fat, file->curr_cluster);
    }

    file->curr_cluster = next_cluster;
  }

  file->pos += bytes;
  return 0;
}

static cluster_t cluster_alloc_free(fat_t *fat, size_t clusters) {
  cluster_t prev = FAT_CLUSTER_INVALID, start = FAT_CLUSTER_INVALID;
  const size_t total =
      fat->sectors_per_fat * fat->bytes_per_sector / sizeof(cluster_t);

  for (cluster_t curr = CLUSTER_START_NO; clusters && (curr < total); curr++) {
    cluster_t next = get_next_cluster(fat, curr);
    if (next == FAT_CLUSTER_FREE) {
      if (!is_cluster_valid(start))
        start = curr;

      if (is_cluster_valid(prev) && set_next_cluster(fat, prev, curr) < 0) {
        // cluster_unlink(fat, start);
        return FAT_CLUSTER_INVALID;
      }

      prev = curr;
      clusters--;
    }
  }

  if (!clusters && !set_next_cluster(fat, prev, FAT_CLUSTER_INVALID))
    return start;

  // cluster_unlink(fat, start);
  return FAT_CLUSTER_INVALID;
}

int expand_file(file_t *file, size_t incr) {
  fat_t *fat = file->fs->data;
  size_t clusters;

  if (!(file->size % fat->bytes_per_cluster)) {
    clusters = up2(incr, fat->bytes_per_cluster) / fat->bytes_per_cluster;
  } else {
    size_t rem = fat->bytes_per_cluster - (file->size % fat->bytes_per_cluster);
    if (rem > incr)
      return 0;

    clusters = up2(incr - rem, fat->bytes_per_cluster) / fat->bytes_per_cluster;
    if (!clusters)
      clusters = 1;
  }

  cluster_t start = cluster_alloc_free(fat, clusters);
  if (!is_cluster_valid(start)) {
    log_printf("No remaining cluster for the new file!");
    return -1;
  }

  if (!is_cluster_valid(file->cluster_start))
    file->cluster_start = start;
  else if (set_next_cluster(fat, file->curr_cluster, file->cluster_start) < 0)
    return -1;

  return 0;
}

int buf_read_sector(fat_t *fat, int sector_no) {
  if (sector_no == fat->curr_sector)
    return 0;

  if (dev_read(fat->fs->dev_id, sector_no, fat->fat_buf, 1) == 1) {
    fat->curr_sector = sector_no;
    return 0;
  }

  return -1;
}

int buf_write_sector(fat_t *fat, int sector_no) {
  return dev_write(fat->fs->dev_id, sector_no, fat->fat_buf, 1) == 1 ? 0 : -1;
}

int fatfs_open(fs_t *fs, const char *path, file_t *file) {
  fat_t *fat = fs->data;
  dirent_t *dirent = NULL;
  int dirent_index = -1;

  for (size_t i = 0; i < fat->root_entries; i++) {
    dirent_t *curr_dirent = read_dirent(fat, i);
    if (!curr_dirent)
      return -1;

    if (*curr_dirent->filename == DIRENT_NAME_END) {
      dirent_index = i;
      break;
    }

    if (*curr_dirent->filename == DIRENT_NAME_FREE) {
      dirent_index = i;
      continue;
    }

    char curr_filename[FAT_FILENAME_LEN + 2]; // '.' & '\0'
    dirent_get_name(curr_dirent, curr_filename);
    if (!kernel_strcmp(kernel_strlwr(curr_filename), path)) {
      dirent = curr_dirent;
      dirent_index = i;
      break;
    }
  }

  /*
   * if - Found a valid directory entry
   * else if - Found a free directory entry, while the file should be created
   */
  if (dirent) {
    read_from_dirent(file, dirent, dirent_index);
    if (file->mode & O_TRUNC) {
      // cluster_unlink(fat, file->cluster_start);
      file->curr_cluster = file->cluster_start = FAT_CLUSTER_INVALID;
      file->size = 0;
    }
  } else if (file->mode & O_CREAT && dirent_index >= 0) {
    dirent_t new_dirent = {
        .first_cluster_hi = FAT_CLUSTER_INVALID >> (CLUSTER_BITS / 2),
        .first_cluster_lo = FAT_CLUSTER_INVALID & 0xFFFF,
        .file_size = 0,
        .file_attr = 0,
        ._reserved = 0,
        .created_date = 0,
        .created_time = 0,
        .last_mod_date = 0,
        .last_mod_time = 0,
        .last_accessed_date = 0,
    };

    kernel_strcpy((char *)new_dirent.filename, file->name);
    if (write_dirent(fat, &new_dirent, dirent_index) < 0) {
      log_printf("Failed to create file!");
      return -1;
    }

    read_from_dirent(file, &new_dirent, dirent_index);
  } else
    return -1;

  return 0;
}

int fatfs_close(file_t *file) {
  if (file->mode == O_RDONLY)
    return 0;

  fat_t *fat = file->fs->data;
  dirent_t *dirent = read_dirent(fat, file->dirent_index);
  if (!dirent)
    return -1;

  dirent->file_size = file->size;
  dirent->first_cluster_hi = file->cluster_start >> (CLUSTER_BITS / 2);
  dirent->first_cluster_lo = file->cluster_start & 0xFF;
  write_dirent(fat, dirent, file->dirent_index);

  return 0;
}

int fatfs_read(void *buf, size_t size, file_t *file) {
  fat_t *fat = file->fs->data;

  if (file->pos + size > file->size)
    size = file->size - file->pos;

  uint32_t read_bytes = 0;
  while (size) {
    uint32_t curr_read_bytes = size;
    const uint32_t cluster_offset = file->pos % fat->bytes_per_cluster;
    const uint32_t start_sector =
        fat->data_start +
        (file->curr_cluster - CLUSTER_START_NO) * fat->sectors_per_cluster;

    /*
     * if - The number of read clusters is a integer, and the starting position
     * is at the end(start) of a cluster.
     *
     * else - The number of read clusters is not a integer, or the starting
     * position is in the middle of a cluster.
     */
    if (!cluster_offset && curr_read_bytes == fat->bytes_per_cluster) {
      if (dev_read(fat->fs->dev_id, start_sector, buf,
                   fat->sectors_per_cluster) < 0) {
        return read_bytes;
      }

      curr_read_bytes = fat->bytes_per_cluster;
    } else {
      if (cluster_offset + curr_read_bytes > fat->bytes_per_cluster)
        curr_read_bytes = fat->bytes_per_cluster - cluster_offset;

      fat->curr_sector = -1;
      if (dev_read(fat->fs->dev_id, start_sector, fat->fat_buf,
                   fat->sectors_per_cluster) < 0) {
        return read_bytes;
      }

      kernel_memcpy(buf, fat->fat_buf + cluster_offset, curr_read_bytes);
    }

    buf += curr_read_bytes;
    size -= curr_read_bytes;
    read_bytes += curr_read_bytes;

    if (move_file_pos(file, fat, curr_read_bytes, FALSE) < 0)
      return read_bytes;
  }

  return read_bytes;
}

int fatfs_write(const void *buf, size_t size, file_t *file) {
  fat_t *fat = file->fs->data;
  if (file->pos + size > file->size)
    if (expand_file(file, file->pos + size - file->size) < 0)
      return -1;

  uint32_t written_bytes = 0;
  while (size) {
    uint32_t curr_written_bytes = size;
    const uint32_t cluster_offset = file->pos % fat->bytes_per_cluster;
    const uint32_t start_sector =
        fat->data_start +
        (file->curr_cluster - CLUSTER_START_NO) * fat->sectors_per_cluster;

    /*
     * if - The number of read clusters is a integer, and the starting position
     * is at the end(start) of a cluster.
     *
     * else - The number of read clusters is not a integer, or the starting
     * position is in the middle of a cluster.
     */
    if (!cluster_offset && curr_written_bytes == fat->bytes_per_cluster) {
      if (dev_write(fat->fs->dev_id, start_sector, buf,
                    fat->sectors_per_cluster) < 0) {
        return written_bytes;
      }

      curr_written_bytes = fat->bytes_per_cluster;
    } else {
      if (cluster_offset + curr_written_bytes > fat->bytes_per_cluster)
        curr_written_bytes = fat->bytes_per_cluster - cluster_offset;

      fat->curr_sector = -1;
      if (dev_read(fat->fs->dev_id, start_sector, fat->fat_buf,
                   fat->sectors_per_cluster) < 0) {
        return written_bytes;
      }

      kernel_memcpy(fat->fat_buf + cluster_offset, buf, curr_written_bytes);
      if (dev_write(fat->fs->dev_id, start_sector, fat->fat_buf,
                    fat->sectors_per_cluster) < 0) {
        return written_bytes;
      }
    }

    buf += curr_written_bytes;
    size -= curr_written_bytes;
    written_bytes += curr_written_bytes;
    file->size += curr_written_bytes;

    if (move_file_pos(file, fat, curr_written_bytes, TRUE) < 0)
      return written_bytes;
  }

  return 0;
}

int fatfs_seek(file_t *file, uint32_t offset, int dir) {
  if (dir)
    return -1;

  fat_t *fat = file->fs->data;
  cluster_t curr_cluster = file->curr_cluster;
  uint32_t curr_pos = 0;
  while (offset) {
    const uint32_t curr_offset = curr_pos % fat->bytes_per_cluster;
    uint32_t curr_moved_bytes = offset;
    if (curr_offset + curr_moved_bytes < fat->bytes_per_cluster) {
      curr_pos += curr_moved_bytes;
      break;
    } // Not cross-cluster

    // Cross-cluster
    curr_moved_bytes = fat->bytes_per_cluster - curr_offset;
    curr_pos += curr_moved_bytes;
    offset -= curr_moved_bytes;
    curr_cluster = get_next_cluster(fat, curr_cluster);
    if (!is_cluster_valid(curr_cluster))
      return -1;
  }

  file->pos = curr_pos;
  file->curr_cluster = curr_cluster;
  return 0;
}

int fatfs_stat(file_t *file, struct stat *stat) { return -1; }

void cluster_unlink(fat_t *fat, const dirent_t *prev, const dirent_t *curr) {
  size_t prev_cluster =
      get_cluster_no(prev) + prev->file_size / fat->bytes_per_cluster;
  size_t curr_cluster = get_cluster_no(curr);
  size_t clusters = curr->file_size / fat->bytes_per_cluster + 1;

  while (clusters--) {
    cluster_t tmp_next = get_next_cluster(fat, curr_cluster);
    set_next_cluster(fat, curr_cluster, FAT_CLUSTER_FREE);
    curr_cluster = tmp_next;
  }

  const dirent_t *next = get_next_dirent(fat, curr);
  set_next_cluster(fat, prev_cluster, get_cluster_no(next));
}

int fatfs_unlink(fs_t *fs, const char *path) {
  fat_t *fat = fs->data;

  dirent_t *prev_dirent = read_dirent(fat, 0);
  dirent_t *curr_dirent = get_next_dirent(fat, prev_dirent);
  if (!prev_dirent)
    return -1; // prev_dirent matches?

  for (size_t i = 0; i < fat->root_entries - 2; i++) {
    if (!prev_dirent || !curr_dirent)
      return -1;

    if (*curr_dirent->filename == DIRENT_NAME_END)
      break;

    if (*curr_dirent->filename == DIRENT_NAME_FREE)
      continue;

    char curr_filename[FAT_FILENAME_LEN + 2]; // '.' & '\0'
    dirent_get_name(curr_dirent, curr_filename);
    if (!kernel_strcmp(kernel_strlwr(curr_filename), path)) {
      cluster_unlink(fat, prev_dirent, curr_dirent);

      dirent_t null_dirent;
      kernel_memset(&null_dirent, 0, sizeof(dirent_t));
      return write_dirent(fat, &null_dirent, i + 1);
    }

    prev_dirent = curr_dirent;
    curr_dirent = get_next_dirent(fat, curr_dirent);
  }

  // When i = fat->root_entries
  return -1;
}

void dirent_get_name(const dirent_t *dirent, char *str_buf) {
  char *filename_ptr = str_buf, *ext_ptr = NULL;
  kernel_memset(str_buf, 0, FAT_FILENAME_LEN + 1); // '.'

  for (int i = 0; i < FAT_FILENAME_LEN; i++) {
    if (dirent->filename[i] != ' ')
      *filename_ptr++ = dirent->filename[i];

    if (i == FAT_BASENAME_LEN - 1) {
      ext_ptr = filename_ptr;
      *filename_ptr++ = '.';
    }
  }

  if (ext_ptr && ext_ptr[1] == '\0')
    *ext_ptr = '\0'; // filename without extension (such as "file.")
}

file_type_t dirent_get_type(const dirent_t *dirent) {
  if (dirent->file_attr & (DIRENT_VOL_ID | DIRENT_HIDDEN | DIRENT_SYS))
    return UNKNOWN_FILE;

  if ((dirent->file_attr & DIRENT_LFN) == DIRENT_LFN)
    return UNKNOWN_FILE;

  return dirent->file_attr & DIRENT_DIR ? DIR_FILE : NORMAL_FILE;
}

dirent_t *read_dirent(fat_t *fat, size_t index) {
  if (index > fat->root_entries)
    return NULL;

  // DBR - FAT1 - FAT2 - Root directory - File data
  const size_t offset = index * sizeof(dirent_t);
  const size_t sector_no = fat->root_start + offset / fat->bytes_per_sector;

  if (buf_read_sector(fat, sector_no) < 0)
    return NULL;

  return (dirent_t *)(fat->fat_buf + offset % fat->bytes_per_sector);
}

int write_dirent(fat_t *fat, const dirent_t *dirent, size_t index) {
  if (index > fat->root_entries)
    return -1;

  // DBR - FAT1 - FAT2 - Root directory - File data
  const size_t offset = index * sizeof(dirent_t);
  const size_t sector_no = fat->root_start + offset / fat->bytes_per_sector;

  const int err = buf_read_sector(fat, sector_no);
  if (err < 0)
    return err;

  kernel_memcpy(fat->fat_buf + offset % fat->bytes_per_sector, dirent,
                sizeof(dirent_t));
  return buf_write_sector(fat, sector_no);
}

int fatfs_opendir(fs_t *fs, const char *name, DIR *dir) {
  dir->index = 0;
  return 0;
}

int fatfs_readdir(fs_t *fs, DIR *dir, struct dirent *dirent) {
  fat_t *fat = fs->data;

  while (dir->index < fat->root_entries) {
    const dirent_t *curr_dirent = read_dirent(fat, dir->index);
    if (!curr_dirent)
      return -1;

    if (*curr_dirent->filename == DIRENT_NAME_END)
      break;

    if (*curr_dirent->filename != DIRENT_NAME_FREE) {
      file_type_t type = dirent_get_type(curr_dirent);
      if (type == NORMAL_FILE || type == DIR_FILE) {
        dirent_get_name(curr_dirent, dirent->name);
        dirent->size = curr_dirent->file_size;
        dirent->type = type;
        dirent->index = dir->index++;
        return 0;
      }
    }

    dir->index++;
  }

  return -1;
}

int fatfs_closedir(fs_t *fs, DIR *dir) { return 0; }
