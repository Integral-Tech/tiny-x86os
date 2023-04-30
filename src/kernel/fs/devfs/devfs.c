#include "fs/devfs/devfs.h"
#include "tools/klib.h"
#include "tools/log.h"

fs_api_t devfs_api = {.mount = devfs_mount,
                      .unmount = devfs_unmount,
                      .open = devfs_open,
                      .close = devfs_close,
                      .read = devfs_read,
                      .write = devfs_write,
                      .seek = devfs_seek,
                      .stat = devfs_stat,
                      .ioctl = devfs_ioctl};

static const devfs_type_t dev_type_table[] = {
    {.name = "tty", .dev_type = TTY_DEV, .file_type = TTY_FILE}};

int devfs_mount(fs_t *fs, int major_no, int minor_no) {
  fs->type = DEVFS;
  return 0;
}

int devfs_unmount(fs_t *fs) { return 0; }

int devfs_open(fs_t *fs, const char *path, file_t *file) {
  for (size_t i = 0; i < sizeof(dev_type_table) / sizeof(devfs_type_t); i++) {
    const devfs_type_t *curr = dev_type_table + i;
    const size_t type_name_len = kernel_strlen(curr->name);
    if (str_begin_with(path, curr->name)) {
      int minor_no;
      if (kernel_strlen(path) == type_name_len ||
          (str2num_dec(path + type_name_len, &minor_no) < 0)) {
        log_printf("Get device minor number failed: Invalid path %s", path);
        break;
      }

      const int dev_id = dev_open(curr->dev_type, minor_no, NULL);
      if (dev_id < 0) {
        log_printf("Open device failed: path = %s", path);
        break;
      }

      file->dev_id = dev_id;
      file->fs = fs;
      file->size = 0;
      file->type = curr->file_type;
      return 0;
    }
  }

  return -1;
}

int devfs_close(file_t *file) { return dev_close(file->dev_id); }

int devfs_read(void *buf, size_t size, file_t *file) {
  return dev_read(file->dev_id, file->pos, buf, size);
}

int devfs_write(const void *buf, size_t size, file_t *file) {
  return dev_write(file->dev_id, file->pos, buf, size);
}

int devfs_seek(file_t *file, uint32_t offset, int dir) { return dir ? -1 : 0; }

int devfs_stat(file_t *file, struct stat *stat) { return -1; }

int devfs_ioctl(file_t *file, int cmd, void *arg0, void *arg1) {
  return dev_control(file->dev_id, cmd, arg0, arg1);
}
