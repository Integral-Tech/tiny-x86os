#include "ipc/mutex.h"
#include "cpu/irq.h"

void mutex_init(mutex_t *mutex) {
  mutex->locked_cnt = 0;
  mutex->owner = NULL;
  list_init(&mutex->wait_list);
}

void mutex_lock(mutex_t *mutex) {
  const irq_state_t state = irq_protect();

  task_t *curr = get_curr_task();
  if (mutex->locked_cnt == 0) { // if the Mutex is available
    mutex->locked_cnt++;
    mutex->owner = curr;
  } else if (mutex->owner == curr)
    mutex->locked_cnt++;
  else { // if the Mutex is owned by other task
    task_set_block(curr);
    list_insert_last(&mutex->wait_list, &curr->wait_node);
    task_dispatch();
  }

  irq_unprotect(state);
}

void mutex_unlock(mutex_t *mutex) {
  const irq_state_t state = irq_protect();

  const task_t *curr = get_curr_task();
  /*
   * The Mutex should only be unlocked by the task which lock the Mutex.
   * If wait_list is not empty, pass the Mutex to the first task of wait_list.
   */
  if (mutex->owner == curr) {
    if (--mutex->locked_cnt == 0) {
      mutex->owner = NULL;

      if (list_cnt(&mutex->wait_list)) {
        const list_node_t *node = list_first(&mutex->wait_list);
        task_t *task = list_node_parent(node, task_t, wait_node);
        mutex->locked_cnt++;
        mutex->owner = task;
        task_set_ready(task);
        task_dispatch();
      }
    }
  }

  irq_unprotect(state);
}
