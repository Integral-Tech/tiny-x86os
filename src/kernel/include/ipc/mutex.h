#ifndef MUTEX_H
#define MUTEX_H

#include "core/task.h"

typedef struct _mutex_t {
  task_t *owner;
  int locked_cnt;
  list_t wait_list;
} mutex_t;

void mutex_init(mutex_t *mutex);
void mutex_lock(mutex_t *mutex);
void mutex_unlock(mutex_t *mutex);

#endif
