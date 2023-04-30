#ifndef SYSCALL_H
#define SYSCALL_H

#define SYSCALL_ARGC 5

#include "comm/types.h"

typedef int (*syscall_handler_t)(void *arg0, void *arg1, void *arg2,
                                 void *arg3);

enum syscall_t {
  SYS_SLEEP,
  SYS_GETPID,
  SYS_PRINTMSG,
  SYS_FORK,
  SYS_EXECVE,
  SYS_YIELD,
  SYS_OPEN,
  SYS_CLOSE,
  SYS_LSEEK,
  SYS_IOCTL,
  SYS_READ,
  SYS_WRITE,
  SYS_ISATTY,
  SYS_FSTAT,
  SYS_SBRK,
  SYS_DUP,
  SYS_UNLINK,
  SYS_EXIT,
  SYS_WAIT,
  SYS_OPENDIR,
  SYS_READDIR,
  SYS_CLOSEDIR,
  SYS_POWEROFF,
  SYS_REBOOT
};

typedef struct _syscall_frame_t {
  struct {
    uint32_t eflags;
    uint16_t gs, _gs, fs, _fs, es, _es, ds, _ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
  } manual_push;

  struct {
    uint32_t eip;
    uint16_t cs, _cs;
    unsigned int func_id;
    struct {
      void *arg0, *arg1, *arg2, *arg3;
    };
    uint32_t esp;
    uint16_t ss, _ss;
  } auto_push;
} syscall_frame_t;

void syscall_handler();

#endif
