// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TASK_H
#define TASK_H

#include "cpu/cpu.h"
#include "fs/file.h"
#include "tools/list.h"

#define TASK_NAME_SIZE 32
#define TASK_TIME_SLICE_DEFAULT 10
#define TASK_FILE_NUM 128

typedef enum _flag_t { SYSTEM, USER } flag_t;

typedef struct _task_args_t {
  uint32_t ret_addr;
  uint32_t argc;
  char **argv;
} task_args_t;

typedef struct _task_t {
  enum {
    TASK_CREATED,
    TASK_RUNNING,
    TASK_SLEEPING,
    TASK_READY,
    TASK_WAITING,
    TASK_ZOMBIE
  } state;

  int pid;
  struct _task_t *parent;

  uint32_t heap_start, heap_end;

  struct {
    int time_ticks;  // maximum ticks occupied by a single task
    int slice_ticks; // ticking timer (initial value is time_ticks)
    int sleep_ticks; // sleeping timer
  };

  char name[TASK_NAME_SIZE];
  file_t *file_table[TASK_FILE_NUM];
  struct {
    list_node_t run_node;  // insert to ready_list/sleep_list
    list_node_t wait_node; // insert to wait_list
    list_node_t all_node;  // insert to task_list
  };

  tss_t tss;
  uint16_t tss_selector;

  int exit_status; // status when the task exits
} task_t;

typedef struct _task_manager_t {
  task_t *curr_task;

  struct {
    list_t ready_list, task_list, sleep_list;
  };

  struct {
    task_t first_task, idle_task; // a task which executes only when CPU is idle
  };

  struct {
    int app_code_selector, app_data_selector;
  };
} task_manager_t;

int task_init(task_t *task, const char *name, flag_t flag, uint32_t entry,
              uint32_t esp);
void task_switch_to(const task_t *target_task);
void task_manager_init();
void task_first_init();
task_t *get_first_task();
task_t *get_curr_task();
void task_set_ready(task_t *task);
void task_set_block(task_t *task);
int sys_yield();
void task_dispatch();
void task_time_tick();

void task_set_sleep(task_t *task, uint32_t ticks);
void task_set_wakeup(task_t *task);
void sys_sleep(uint32_t sleeping_time);
int sys_getpid();
void sys_print_msg(const char *fmt, int arg); // for debug temporarily
int sys_fork();
int sys_execve(const char *name, char *const argv[], char *const envp[]);
void sys_exit(int status);

int task_alloc_fd(file_t *file);
int task_remove_fd(int fd);
file_t *task_file(int fd);

int sys_wait(int *status);
#endif
