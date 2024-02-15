// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ipc/sem.h"
#include "core/task.h"
#include "cpu/irq.h"

void sem_init(sem_t *sem, int init_cnt) {
  sem->count = init_cnt;
  list_init(&sem->wait_list);
}

void sem_wait(sem_t *sem) {
  const irq_state_t state = irq_protect();

  if (sem->count > 0)
    sem->count--;
  else {
    task_t *curr = get_curr_task();
    task_set_block(curr);
    list_insert_last(&sem->wait_list, &curr->wait_node);
    task_dispatch();
  }

  irq_unprotect(state);
}

void sem_notify(sem_t *sem) {
  const irq_state_t state = irq_protect();

  if (list_cnt(&sem->wait_list)) {
    list_node_t *curr = list_first(&sem->wait_list);
    list_remove(&sem->wait_list, curr);
    task_set_ready(list_node_parent(curr, task_t, wait_node));
    task_dispatch();
  } else
    sem->count++;

  irq_unprotect(state);
}

int sem_cnt(const sem_t *sem) { return sem->count; }
