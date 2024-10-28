// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/syscall.h"
#include "acpi/poweroff.h"
#include "acpi/reboot.h"
#include "core/memory.h"
#include "fs/fs.h"
#include "tools/log.h"
#include "tools/klib.h"

static const syscall_handler_t sys_table[] = {
    [SYS_SLEEP] = (syscall_handler_t)sys_sleep,
    [SYS_GETPID] = (syscall_handler_t)sys_getpid,
    [SYS_PRINTMSG] = (syscall_handler_t)sys_print_msg,
    [SYS_FORK] = (syscall_handler_t)sys_fork,
    [SYS_EXECVE] = (syscall_handler_t)sys_execve,
    [SYS_YIELD] = (syscall_handler_t)sys_yield,
    [SYS_OPEN] = (syscall_handler_t)sys_open,
    [SYS_CLOSE] = (syscall_handler_t)sys_close,
    [SYS_LSEEK] = (syscall_handler_t)sys_lseek,
    [SYS_IOCTL] = (syscall_handler_t)sys_ioctl,
    [SYS_READ] = (syscall_handler_t)sys_read,
    [SYS_WRITE] = (syscall_handler_t)sys_write,
    [SYS_ISATTY] = (syscall_handler_t)sys_isatty,
    [SYS_FSTAT] = (syscall_handler_t)sys_fstat,
    [SYS_SBRK] = (syscall_handler_t)sys_sbrk,
    [SYS_DUP] = (syscall_handler_t)sys_dup,
    [SYS_EXIT] = (syscall_handler_t)sys_exit,
    [SYS_WAIT] = (syscall_handler_t)sys_wait,
    [SYS_OPENDIR] = (syscall_handler_t)sys_opendir,
    [SYS_READDIR] = (syscall_handler_t)sys_readdir,
    [SYS_CLOSEDIR] = (syscall_handler_t)sys_closedir,
    [SYS_POWEROFF] = (syscall_handler_t)sys_poweroff,
    [SYS_REBOOT] = (syscall_handler_t)sys_reboot,
    [SYS_UNLINK] = (syscall_handler_t)sys_unlink};

void do_handle_syscall(syscall_frame_t *frame) {
  if (frame->auto_push.func_id < ARRAY_SIZE(sys_table)) {
    const syscall_handler_t handler = sys_table[frame->auto_push.func_id];
    if (handler) {
      const int ret = handler(frame->auto_push.arg0, frame->auto_push.arg1,
                              frame->auto_push.arg2, frame->auto_push.arg3);
      frame->manual_push.eax = ret;
      return;
    }
  }
  const task_t *task = get_curr_task();
  log_printf("Task: %s, Unknown syscall: %d", task->name,
             frame->auto_push.func_id);
  frame->manual_push.eax = -1;
}

int sys_wait(int *status);
