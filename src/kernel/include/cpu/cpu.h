#ifndef CPU_H
#define CPU_H

#include "comm/types.h"
#pragma pack(1)

typedef struct _segment_desc_t {
  uint16_t limit15_0;
  uint16_t base15_0;
  uint8_t base23_16;
  uint16_t attr;
  uint8_t base31_24;
} segment_desc_t;

typedef struct _gate_desc_t {
  uint16_t offset15_0;
  uint16_t selector;
  uint16_t attr;
  uint16_t offset31_16;
} gate_desc_t;

typedef struct _tss_t {
  uint32_t pre_link;
  uint32_t esp0, ss0, esp1, ss1, esp2, ss2;
  uint32_t cr3;
  uint32_t eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
  uint32_t es, cs, ss, ds, fs, gs;
  uint32_t ldt_selector;
  uint32_t io_map;
} tss_t;

#pragma pack()

#define SEG_G (1 << 15) // G(Granularity) bit
#define SEG_D (1 << 14) // D(Default operation size) bit
#define SEG_P (1 << 7)  // P(Present) bit

#define SEG_DPL0 (0 << 5)
#define SEG_DPL3 (3 << 5) // Descriptor Privilege Level
#define SEG_CPL0 (0 << 0)
#define SEG_CPL3 (3 << 0) // Current Privilege Level
#define SELECTOR_RPL0 (0 << 0)

#define SEG_SYSTEM (0 << 4) // Descriptor Type = System
#define SEG_NORMAL (1 << 4) // Descriptor Type = Normal

#define SEG_TYPE_CODE (1 << 3)
#define SEG_TYPE_DATA (0 << 3)
#define SEG_TYPE_TSS (9 << 0)

#define SEG_TYPE_RW (1 << 1) // Readable & Writable

#define GATE_P (1 << 15) // Present Bit
#define GATE_DPL0 (0 << 13)
#define GATE_DPL3 (3 << 13)
#define GATE_TYPE_INT (0xE << 8) // D=1 (32 bit)
#define GATE_TYPE_SYSCALL (0xC << 8)

#define EFLAGS_DEFAULT (1 << 1)
#define EFLAGS_IF (1 << 9)

void cpu_init();
void segment_desc_set(int selector, uint32_t base, uint32_t limit,
                      uint16_t attr);
void gate_desc_set(gate_desc_t *desc, uint16_t selector, uint32_t offset,
                   uint16_t attr);
int gdt_alloc_desc();
void switch_to_tss(int tss_selector);
void gdt_free_selector(int selector);

#endif