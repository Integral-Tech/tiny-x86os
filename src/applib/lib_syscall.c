// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "lib_syscall.h"
#include "comm/cpu_instr.h"
#include "core/syscall.h"
#include "os_cfg.h"
#include <stdlib.h>

int sys_call(const syscall_args_t *args) {
  const uint32_t addr[] = {0, SYSCALL_SELECTOR | SELECTOR_RPL0};
  int ret;
  __asm__ __volatile__(
      "push %[arg3]\n\t"
      "push %[arg2]\n\t"
      "push %[arg1]\n\t"
      "push %[arg0]\n\t"
      "push %[id]\n\t"
      "lcalll *(%[a])"
      : "=a"(ret)
      : [arg3] "r"(args->arg3), [arg2] "r"(args->arg2), [arg1] "r"(args->arg1),
        [arg0] "r"(args->arg0), [id] "r"(args->id), [a] "r"(addr));
  return ret;
}

void msleep(uint32_t time) {
  if (time <= 0)
    return;

  syscall_args_t args = {.id = SYS_SLEEP, .arg0 = (void *)time};
  sys_call(&args);
}

int getpid() {
  syscall_args_t args = {.id = SYS_GETPID};
  return sys_call(&args);
}

void print_msg(const char *fmt, int arg) {
  syscall_args_t args = {
      .id = SYS_PRINTMSG, .arg0 = (void *)fmt, .arg1 = (void *)arg};
  sys_call(&args);
} // for debug temporarily

int fork() {
  syscall_args_t args = {.id = SYS_FORK};
  return sys_call(&args);
}

int execve(const char *name, char *const argv[], char *const envp[]) {
  syscall_args_t args = {.id = SYS_EXECVE,
                         .arg0 = (void *)name,
                         .arg1 = (void *)argv,
                         .arg2 = (void *)envp};
  return sys_call(&args);
}

int yield() {
  syscall_args_t args = {.id = SYS_YIELD};
  return sys_call(&args);
}

int open(const char *path, int oflag, ...) {
  syscall_args_t args = {
      .id = SYS_OPEN, .arg0 = (void *)path, .arg1 = (void *)oflag};
  return sys_call(&args);
}

ssize_t read(int fd, void *buf, size_t nbytes) {
  syscall_args_t args = {
      .id = SYS_READ, .arg0 = (void *)fd, .arg1 = buf, .arg2 = (void *)nbytes};
  return sys_call(&args);
}

ssize_t write(int fd, const void *buf, size_t nbytes) {
  syscall_args_t args = {.id = SYS_WRITE,
                         .arg0 = (void *)fd,
                         .arg1 = (void *)buf,
                         .arg2 = (void *)nbytes};
  return sys_call(&args);
}

int close(int fd) {
  syscall_args_t args = {.id = SYS_CLOSE, .arg0 = (void *)fd};
  return sys_call(&args);
}

int lseek(int fd, int offset, int whence) {
  syscall_args_t args = {.id = SYS_LSEEK,
                         .arg0 = (void *)fd,
                         .arg1 = (void *)offset,
                         .arg2 = (void *)whence};
  return sys_call(&args);
}

int ioctl(int fd, int cmd, void *arg0, void *arg1) {
  syscall_args_t args = {.id = SYS_IOCTL,
                         .arg0 = (void *)fd,
                         .arg1 = (void *)cmd,
                         .arg2 = arg0,
                         .arg3 = arg1};
  return sys_call(&args);
}

int isatty(int fd) {
  syscall_args_t args = {.id = SYS_ISATTY, .arg0 = (void *)fd};
  return sys_call(&args);
}

int fstat(int fd, struct stat *buf) {
  syscall_args_t args = {.id = SYS_FSTAT, .arg0 = (void *)fd, .arg1 = buf};
  return sys_call(&args);
}

void *sbrk(ptrdiff_t incr) {
  syscall_args_t args = {.id = SYS_SBRK, .arg0 = (void *)incr};
  return (void *)sys_call(&args);
}

int dup(int fd) {
  syscall_args_t args = {.id = SYS_DUP, .arg0 = (void *)fd};
  return sys_call(&args);
}

int unlink(const char *pathname) {
  syscall_args_t args = {.id = SYS_UNLINK, .arg0 = (void *)pathname};
  return sys_call(&args);
}

void _exit(int status) {
  syscall_args_t args = {.id = SYS_EXIT, .arg0 = (void *)status};
  sys_call(&args);
  while (1)
    hlt();
}

int wait(int *status) {
  syscall_args_t args = {.id = SYS_WAIT, .arg0 = status};
  return sys_call(&args);
}

DIR *opendir(const char *name) {
  DIR *dir = (DIR *)malloc(sizeof(DIR));
  if (!dir)
    return NULL;

  syscall_args_t args = {.id = SYS_OPENDIR, .arg0 = (void *)name, .arg1 = dir};

  if (sys_call(&args) < 0) {
    free(dir);
    return NULL;
  }

  return dir;
}

struct dirent *readdir(DIR *dir) {
  syscall_args_t args = {.id = SYS_READDIR, .arg0 = dir, .arg1 = &dir->dirent};
  // Save the read data to dir->dirent

  if (sys_call(&args) < 0)
    return NULL;

  return &dir->dirent;
}

int closedir(DIR *dir) {
  syscall_args_t args = {.id = SYS_CLOSEDIR, .arg0 = dir};
  if (sys_call(&args) < 0) {
    free(dir);
    return -1;
  }

  free(dir);
  return 0;
}

int poweroff() {
  syscall_args_t args = {.id = SYS_POWEROFF};
  return sys_call(&args);
}

int reboot() {
  syscall_args_t args = {.id = SYS_REBOOT};
  return sys_call(&args);
}
