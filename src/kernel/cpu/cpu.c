// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cpu/cpu.h"
#include "comm/cpu_instr.h"
#include "core/syscall.h"
#include "ipc/mutex.h"
#include "os_cfg.h"

static segment_desc_t gdt_table[GDT_TABLE_SIZE];
static mutex_t mutex;

void segment_desc_set(int selector, uint32_t base, uint32_t limit,
                      uint16_t attr) {
  if (limit > 0xFFFFF) {
    attr |= 0x8000; // set G(Granularity) bit
    limit >>= 12;   // unit = 4KB
  }

  segment_desc_t *desc = gdt_table + selector / sizeof(segment_desc_t);
  desc->limit15_0 = limit & 0xFFFF;
  desc->base15_0 = base & 0xFFFF;
  desc->base23_16 = (base >> 16) & 0xFF;
  desc->attr = attr | (((limit >> 16) & 0xF) << 8);
  desc->base31_24 = base >> 24;
}

void gate_desc_set(gate_desc_t *desc, uint16_t selector, uint32_t offset,
                   uint16_t attr) {
  desc->offset15_0 = offset & 0xFFFF;
  desc->selector = selector;
  desc->attr = attr;
  desc->offset31_16 = offset >> 16;
}

void init_gdt() {
  for (int i = 0; i < GDT_TABLE_SIZE; i++) {
    segment_desc_set(i * sizeof(segment_desc_t), 0, 0, 0);
  }

  segment_desc_set(KERNEL_SELECTOR_CS, 0, 0xFFFFFFFF,
                   SEG_P | SEG_DPL0 | SEG_NORMAL | SEG_TYPE_CODE | SEG_TYPE_RW |
                       SEG_D);
  segment_desc_set(KERNEL_SELECTOR_DS, 0, 0xFFFFFFFF,
                   SEG_P | SEG_DPL0 | SEG_NORMAL | SEG_TYPE_DATA | SEG_TYPE_RW |
                       SEG_D);

  gate_desc_set((gate_desc_t *)(gdt_table + (SYSCALL_SELECTOR >> 3)),
                KERNEL_SELECTOR_CS, (uint32_t)syscall_handler,
                GATE_P | GATE_DPL3 | GATE_TYPE_SYSCALL | SYSCALL_ARGC);

  lgdt((uint32_t)gdt_table, sizeof(gdt_table));
}

void cpu_init() {
  mutex_init(&mutex);
  init_gdt();
}

int gdt_alloc_desc() {
  mutex_lock(&mutex);

  // skip the 0th item (because the 0th item in GDT is null)
  for (int i = 1; i < GDT_TABLE_SIZE; i++) {
    const segment_desc_t *desc = gdt_table + i;
    if (!desc->attr) {
      mutex_unlock(&mutex);
      return i * sizeof(segment_desc_t);
    } // return the selector of the empty item
  }   // find an empty item in GDT

  mutex_unlock(&mutex);
  return -1; // fail to find -> return -1
}

void switch_to_tss(int tss_selector) { far_jump(tss_selector, 0); }

void gdt_free_selector(int selector) {
  mutex_lock(&mutex);
  gdt_table[selector / sizeof(segment_desc_t)].attr = 0;
  mutex_unlock(&mutex);
}
