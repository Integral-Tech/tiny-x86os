// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LIB_SYSCALL_H
#define LIB_SYSCALL_H

#include "fs/file.h"
#include <sys/stat.h>

typedef struct _syscall_args_t {
  int id;

  struct {
    void *arg0, *arg1, *arg2, *arg3;
  };
} syscall_args_t;

int sys_call(const syscall_args_t *args);
void msleep(uint32_t time);
int getpid();
void print_msg(const char *fmt, int arg);
int fork();
int execve(const char *name, char *const argv[], char *const envp[]);
int yield();
int open(const char *path, int oflag, ...);
ssize_t read(int fd, void *buf, size_t nbytes);
ssize_t write(int fd, const void *buf, size_t nbytes);
int close(int fd);
int lseek(int fd, int offset, int whence);
int ioctl(int fd, int cmd, void *arg0, void *arg1);

int isatty(int fd);
int fstat(int fd, struct stat *buf);
void *sbrk(ptrdiff_t incr);
int dup(int fd);
int unlink(const char *pathname);

void _exit(int status);
int wait(int *status);

int poweroff();
int reboot();

#define DIRENT_NAME_LEN 255

struct dirent {
  char name[DIRENT_NAME_LEN];
  file_type_t type;
  size_t index, size;
};

typedef struct _DIR {
  size_t index;
  struct dirent dirent;
} DIR;

DIR *opendir(const char *name);
struct dirent *readdir(DIR *dir);
int closedir(DIR *dir);

#endif
