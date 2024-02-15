// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef CPU_INSTR_H
#define CPU_INSTR_H
#include "core/task.h"

#define cli() __asm__ __volatile__("cli");
#define sti() __asm__ __volatile__("sti");

static inline uint8_t inb(uint16_t port) {
  uint8_t rv; // returned value
  __asm__ __volatile__("inb %[p],%[v]"
                       : [v] "=a"(rv)
                       : [p] "d"(port)); // inb al,dx
  return rv;
}

static inline uint16_t inw(uint16_t port) {
  uint16_t rv; // returned value
  __asm__ __volatile__("in %[p],%[v]"
                       : [v] "=a"(rv)
                       : [p] "d"(port)); // in ax,dx
  return rv;
}

static inline void outb(uint16_t port, uint8_t data) {
  __asm__ __volatile__(
      "outb %[v],%[p]" ::[p] "d"(port), [v] "a"(data)); // outb al,dx
}

static inline void outw(uint16_t port, uint16_t data) {
  __asm__ __volatile__(
      "out %[v],%[p]" ::[p] "d"(port), [v] "a"(data)); // out ax,dx
}

static inline void lgdt(uint32_t start, uint32_t size) {
  struct {
    uint16_t limit;
    uint16_t start15_0;  // the lower 16 bit of the start address
    uint16_t start31_16; // the higher 16 bit of the start address
  } gdt;

  gdt.start31_16 = start >> 16;
  gdt.start15_0 = start & 0xFFFF;
  gdt.limit = size - 1;

  __asm__ __volatile__("lgdt %[g]" ::[g] "m"(gdt));
}

static inline void lidt(uint32_t start, uint32_t size) {
  struct {
    uint16_t limit;
    uint16_t start15_0;
    uint16_t start31_16;
  } idt;

  idt.limit = size - 1;
  idt.start15_0 = start & 0xFFFF;
  idt.start31_16 = start >> 16;
  __asm__ __volatile__("lidt %[i]" ::[i] "m"(idt));
}

static inline uint32_t read_cr0() {
  uint32_t cr0;
  __asm__ __volatile__("mov %%cr0,%[v]" : [v] "=r"(cr0));
  return cr0;
}

#define write_cr0(val) __asm__ __volatile__("mov %[v],%%cr0" ::[v] "r"(val));

static inline uint32_t read_cr2() {
  uint32_t cr2;
  __asm__ __volatile__("mov %%cr2,%[v]" : [v] "=r"(cr2));
  return cr2;
}

#define write_cr2(val) __asm__ __volatile__("mov %[v],%%cr2" ::[v] "r"(val));

static inline uint32_t read_cr3() {
  uint32_t cr3;
  __asm__ __volatile__("mov %%cr3,%[v]" : [v] "=r"(cr3));
  return cr3;
}

#define write_cr3(val) __asm__ __volatile__("mov %[v],%%cr3" ::[v] "r"(val));

static inline uint32_t read_cr4() {
  uint32_t cr4;
  __asm__ __volatile__("mov %%cr4,%[v]" : [v] "=r"(cr4));
  return cr4;
}

#define write_cr4(val) __asm__ __volatile__("mov %[v],%%cr4" ::[v] "r"(val));

static inline void far_jump(uint32_t selector, uint32_t offset) {
  uint32_t addr[] = {offset, selector};
  __asm__ __volatile__("ljmpl *(%[a])" ::[a] "r"(addr));
}

#define jump(eip) __asm__ __volatile__("jmp *%[ip]" ::[ip] "r"(eip));
#define hlt() __asm__ __volatile__("hlt");
#define write_tr(selector) __asm__ __volatile__("ltr %%ax" ::"a"(selector));

static inline uint32_t read_eflags() {
  uint32_t eflags;

  __asm__ __volatile__("pushf\n\tpop %%eax" : "=a"(eflags));
  return eflags;
}

#define write_eflags(eflags)                                                   \
  __asm__ __volatile__("push %%eax\n\tpopf" ::"a"(eflags));

static inline void switch_to_cpl3(const tss_t *tss) {
  __asm__ __volatile__("push %[ss]\n\t"
                       "push %[esp]\n\t"
                       "push %[eflags]\n\t"
                       "push %[cs]\n\t"
                       "push %[eip]\n\t"
                       "iret" ::[ss] "r"(tss->ss),
                       [esp] "r"(tss->esp), [eflags] "r"(tss->eflags),
                       [cs] "r"(tss->cs), [eip] "r"(tss->eip));
}

#endif
