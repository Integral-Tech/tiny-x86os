// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "comm/cpu_instr.h"
#include "core/memory.h"
#include "dev/disk.h"
#include "dev/timer.h"
#include "fs/fs.h"
#include "tools/klib.h"
#include "tools/log.h"

void kernel_init(const boot_info_t *boot_info) {
  ASSERT(boot_info->ram_regions != 0);

  cpu_init();
  irq_init();
  log_init();

  memory_init(boot_info);
  disk_init();
  fs_init();

  time_init();
  task_manager_init();
}

void jump_to_first_task() {
  const task_t *curr = get_curr_task();
  ASSERT(curr != 0);

  const tss_t *tss = &(curr->tss);
  switch_to_cpl3(tss);
}

void init_main() {
  log_printf("Kernel is running...");

  task_first_init();
  jump_to_first_task();
}
