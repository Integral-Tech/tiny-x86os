// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dev/timer.h"
#include "comm/cpu_instr.h"
#include "cpu/irq.h"
#include "os_cfg.h"

static uint32_t sys_tick;

void do_handle_time(const exception_frame_t *frame) {
  sys_tick++;
  pic_send_eoi(IRQ0_TIMER);
  task_time_tick();
}

static void init_pit() {
  const uint32_t reload_cnt = PIT_OSC_FREQ * OS_TICKS_MS / 1000;
  outb(PIT_COMMAND_MODE_PORT, PIT_CHANNEL0 | PIT_LOAD_LOHI | PIT_MODE3);
  outb(PIT_CHANNEL0_DATA_PORT, reload_cnt & 0xFF);        // load lower 8 bit
  outb(PIT_CHANNEL0_DATA_PORT, (reload_cnt >> 8) & 0xFF); // load higher 8 bit

  irq_install(IRQ0_TIMER, (irq_handler_t)exception_handler_time);
  irq_enable(IRQ0_TIMER);
}

void time_init() {
  sys_tick = 0;
  init_pit();
}
