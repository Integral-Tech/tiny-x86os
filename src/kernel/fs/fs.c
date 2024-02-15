// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "fs/fs.h"
#include "dev/dev.h"
#include "os_cfg.h"
#include "tools/klib.h"
#include "tools/log.h"
#include <sys/file.h>

static list_t mounted_list;
static fs_t fs_table[FS_TABLE_SIZE];
static list_t free_list;

static fs_t *root_fs;
extern fs_api_t devfs_api;
extern fs_api_t fatfs_api;

static void fs_protect(fs_t *fs) {
  if (fs->mutex)
    mutex_lock(fs->mutex);
}

static void fs_unprotect(fs_t *fs) {
  if (fs->mutex)
    mutex_unlock(fs->mutex);
}

int sys_open(const char *path, flag_t flag, ...) {
  file_t *file = file_alloc();
  if (!file)
    return -1;

  int fd = -1;
  if ((fd = task_alloc_fd(file)) < 0)
    goto open_failed;

  fs_t *fs = NULL;
  for (list_node_t *node = list_first(&mounted_list); node;
       node = list_node_next(node)) {
    fs_t *curr = list_node_parent(node, fs_t, node);
    if (str_begin_with(path, curr->mount_point)) {
      fs = curr;
      break;
    }
  } // Check whether the file system of path is mounted

  if (fs)
    path = kernel_basename(path);
  else
    fs = root_fs; // Use root_fs as the default file system

  kernel_strncpy(file->name, path, FILENAME_SIZE);
  file->mode = flag;
  file->fs = fs;

  fs_protect(fs);
  if (fs->fs_api->open(fs, path, file) < 0) {
    fs_unprotect(fs);
    goto open_failed;
  }

  fs_unprotect(fs);
  return fd;
open_failed:
  file_free(file);
  if (fd >= 0)
    task_remove_fd(fd);

  return -1;
}

ssize_t sys_read(int fd, void *buf, size_t len) {
  file_t *file = task_file(fd);
  if (!file || !buf || !len)
    return -1;

  fs_t *fs = file->fs;
  fs_protect(fs);
  const int err = fs->fs_api->read(buf, len, file);
  fs_unprotect(fs);
  return err;
}

ssize_t sys_write(int fd, const void *buf, size_t len) {
  file_t *file = task_file(fd);
  if (!file || !buf || !len)
    return -1;

  if (file->mode == O_RDONLY) {
    log_printf("File is read-only!");
    return -1;
  }

  fs_t *fs = file->fs;
  fs_protect(fs);
  const int err = fs->fs_api->write(buf, len, file);
  fs_unprotect(fs);
  return err;
}

int sys_lseek(int fd, int offset, int whence) {
  file_t *file = task_file(fd);
  if (!file) {
    log_printf("File descriptor is invalid or file is not opened!");
    return -1;
  }

  fs_t *fs = file->fs;
  fs_protect(fs);
  const int err = fs->fs_api->seek(file, offset, whence);
  fs_unprotect(fs);
  return err;
}

int sys_close(int fd) {
  file_t *file = task_file(fd);
  if (!file) {
    log_printf("File descriptor is invalid or file is not opened!");
    return -1;
  }

  ASSERT(file->ref > 0);
  if (--file->ref == 0) {
    fs_t *fs = file->fs;
    fs_protect(fs);
    fs->fs_api->close(file);
    fs_unprotect(fs);
    file_free(file);
  }

  task_remove_fd(fd);
  return 0;
}

int sys_ioctl(int fd, int cmd, void *arg0, void *arg1) {
  file_t *file = task_file(fd);
  if (!file) {
    log_printf("The dup system call for fd = %d is invalid!", fd);
    return -1;
  }

  fs_t *fs = file->fs;
  fs_protect(fs);
  if (!fs->fs_api->ioctl)
    return -1;

  const int err = fs->fs_api->ioctl(file, cmd, arg0, arg1);
  fs_unprotect(fs);
  return err;
}

int sys_isatty(int fd) {
  file_t *file = task_file(fd);
  if (!file) {
    log_printf("File descriptor is invalid or file is not opened!");
    return -1;
  }

  return file->type == TTY_FILE;
}

int sys_fstat(int fd, struct stat *buf) {
  file_t *file = task_file(fd);
  if (!file) {
    log_printf("File descriptor is invalid or file is not opened!");
    return -1;
  }

  fs_t *fs = file->fs;
  kernel_memset(buf, 0, sizeof(struct stat));
  fs_protect(fs);
  const int err = fs->fs_api->stat(file, buf);
  fs_unprotect(fs);
  return err;
}

static void mounted_list_init() {
  list_init(&free_list);
  for (int i = 0; i < FS_TABLE_SIZE; i++)
    list_insert_last(&free_list, &fs_table[i].node);

  list_init(&mounted_list);
}

static fs_api_t *get_fs_api(fs_type_t type) {
  switch (type) {
  case DEVFS:
    return &devfs_api;
  case FAT16:
    return &fatfs_api;
  default:
    return NULL;
  }
}

static fs_t *mount(fs_type_t type, char *mnt_point, int major_no,
                   int minor_no) {
  fs_t *fs = NULL;
  log_printf("Mounting file system...");
  log_printf("Mountpoint: %s, Dev: %d", mnt_point, major_no);

  for (list_node_t *curr = list_first(&mounted_list); curr;
       curr = list_node_next(curr)) {
    const fs_t *fs = list_node_parent(curr, fs_t, node);
    if (!kernel_strncmp(fs->mount_point, mnt_point, MOUNTPOINT_SIZE)) {
      log_printf("Mountpoint %s is busy!", mnt_point);
      goto mount_failed;
    }
  } // Examine whether the mountpoint is busy

  const list_node_t *free_node = list_remove_first(&free_list);
  if (!free_node) {
    log_printf("Available file system not found!");
    goto mount_failed;
  }

  fs = list_node_parent(free_node, fs_t, node);
  kernel_memset(fs, 0, sizeof(fs_t));
  kernel_strcpy(fs->mount_point, mnt_point);
  if (!(fs->fs_api = get_fs_api(type))) {
    log_printf("Unsupported file system type: %d", type);
    goto mount_failed;
  }

  if (fs->fs_api->mount(fs, major_no, minor_no) < 0) {
    log_printf("Mount file system on %s failed!", mnt_point);
    goto mount_failed;
  }

  list_insert_last(&mounted_list, &fs->node);
  return fs;
mount_failed:
  if (fs)
    list_insert_last(&free_list, &fs->node);

  return NULL;
}

void fs_init() {
  mounted_list_init();
  file_table_init();

  fs_t *fs = mount(DEVFS, "/dev", 0, 0);
  ASSERT(fs != NULL);

  root_fs = mount(FAT16, "/home", ROOT_DEV);
  ASSERT(root_fs != NULL);
}

int sys_dup(int fd) {
  file_t *file = task_file(fd);
  if (!file) {
    log_printf("The dup system call for fd = %d is invalid!", fd);
    return -1;
  }

  const int new_fd = task_alloc_fd(file);
  if (new_fd >= 0) {
    file_ref_inc(file);
    return new_fd;
  }

  log_printf("Unable to allocate new file descriptor for current task: "
             "available task file does not exists!");
  return -1;
}

int sys_unlink(const char *pathname) {
  fs_protect(root_fs);
  const int err = root_fs->fs_api->unlink(root_fs, pathname);
  fs_unprotect(root_fs);
  return err;
}

int sys_opendir(const char *name, DIR *dir) {
  fs_protect(root_fs);
  const int err = root_fs->fs_api->opendir(root_fs, name, dir);
  fs_unprotect(root_fs);
  return err;
}

int sys_readdir(DIR *dir, struct dirent *dirent) {
  fs_protect(root_fs);
  const int err = root_fs->fs_api->readdir(root_fs, dir, dirent);
  fs_unprotect(root_fs);
  return err;
}

int sys_closedir(DIR *dir) {
  fs_protect(root_fs);
  const int err = root_fs->fs_api->closedir(root_fs, dir);
  fs_unprotect(root_fs);
  return err;
}
