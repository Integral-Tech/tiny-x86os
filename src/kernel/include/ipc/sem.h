// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SEM_H
#define SEM_H

#include "tools/list.h"

typedef struct _sem_t {
  int count;
  list_t wait_list;
} sem_t;

void sem_init(sem_t *sem, int init_cnt);
void sem_wait(sem_t *sem);
void sem_notify(sem_t *sem);
int sem_cnt(const sem_t *sem);

#endif
