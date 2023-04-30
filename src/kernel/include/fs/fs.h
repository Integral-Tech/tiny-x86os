#ifndef FS_H
#define FS_H

#include "applib/lib_syscall.h"
#include "fs/fatfs/fatfs.h"

#define MOUNTPOINT_SIZE 512
#define FS_TABLE_SIZE 10

#define DIRENT_NAME_LEN 255

typedef enum _fs_type_t { DEVFS, FAT16 } fs_type_t;

typedef struct _fs_t fs_t;

typedef struct _fs_api_t {
  int (*mount)(fs_t *fs, int major_no, int minor_no);
  int (*unmount)(fs_t *fs);
  int (*open)(fs_t *fs, const char *path, file_t *file);
  int (*close)(file_t *file);

  int (*read)(void *buf, size_t size, file_t *file);
  int (*write)(const void *buf, size_t size, file_t *file);
  int (*seek)(file_t *file, uint32_t offset, int dir);
  int (*stat)(file_t *file, struct stat *stat);
  int (*ioctl)(file_t *file, int cmd, void *arg0, void *arg1);
  int (*unlink)(fs_t *fs, const char *path);

  int (*opendir)(fs_t *fs, const char *name, DIR *dir);
  int (*readdir)(fs_t *fs, DIR *dir, struct dirent *dirent);
  int (*closedir)(fs_t *fs, DIR *dir);
} fs_api_t;

typedef struct _fs_t {
  char mount_point[MOUNTPOINT_SIZE];
  fs_type_t type;
  fs_api_t *fs_api;
  void *data;
  int dev_id;

  list_node_t node;

  union {
    struct _fat_t fat_data;
  };
  mutex_t *mutex;
} fs_t;

int sys_open(const char *path, flag_t flag, ...);
ssize_t sys_read(int fd, void *buf, size_t len);
ssize_t sys_write(int fd, const void *buf, size_t len);
int sys_lseek(int fd, int offset, int whence);
int sys_close(int fd);
int sys_ioctl(int fd, int cmd, void *arg0, void *arg1);

int sys_isatty(int fd);
int sys_fstat(int fd, struct stat *buf);
int sys_dup(int fd);
int sys_unlink(const char *pathname);

void fs_init();

int sys_opendir(const char *name, DIR *dir);
int sys_readdir(DIR *dir, struct dirent *dirent);
int sys_closedir(DIR *dir);

#endif
