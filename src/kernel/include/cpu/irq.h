// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef IRQ_H
#define IRQ_H

#include "comm/types.h"

typedef enum _irq_t {
  IRQ0_DE,
  IRQ1_DB,
  IRQ2_NMI,
  IRQ3_BP,
  IRQ4_OF,
  IRQ5_BR,
  IRQ6_UD,
  IRQ7_NM,
  IRQ8_DF,
  IRQ10_TS = 10,
  IRQ11_NP,
  IRQ12_SS,
  IRQ13_GP,
  IRQ14_PF,
  IRQ16_MF = 16,
  IRQ17_AC,
  IRQ18_MC,
  IRQ19_XM,
  IRQ20_VE,
  IRQ21_CP
} irq_t;

#define IRQ0_TIMER 0x20
#define IRQ1_KEYBOARD 0x21

#define IRQ14_IDE_PRIMARY (0x20 + 14)

#define PIC0_ICW1 0x20
#define PIC0_ICW2 0x21
#define PIC0_ICW3 0x21
#define PIC0_ICW4 0x21
#define PIC0_IMR 0x21
#define PIC0_OCW2 0x20

#define PIC1_ICW1 0xA0
#define PIC1_ICW2 0xA1
#define PIC1_ICW3 0xA1
#define PIC1_ICW4 0xA1
#define PIC1_IMR 0xA1
#define PIC1_OCW2 0xA0

#define PIC_ICW1_ALWAYS_1 (1 << 4)
#define PIC_ICW1_ICW4 (1 << 0)
#define PIC_ICW4_8086 (1 << 0)
#define PIC_OCW2_EOI (1 << 5)
#define IRQ_PIC_START 0x20

#define ERR_PAGE_P (1 << 0)
#define ERR_PAGE_WR (1 << 1)
#define ERR_PAGE_US (1 << 2)

#define ERR_EXT (1 << 0)
#define ERR_IDT (1 << 1)

typedef struct _exception_frame_t {
  uint16_t gs, _gs, fs, _fs, es, _es, ds, _ds;
  uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // pusha
  uint32_t num, err_code;
  uint32_t eip;
  uint16_t cs, _cs;
  uint32_t eflags;
  uint32_t esp3;
  uint16_t ss3, _ss3;
} exception_frame_t;

typedef void (*irq_handler_t)(const exception_frame_t *frame);
void irq_init();

void exception_handler_unknown();
void exception_handler_divide_error();
void exception_handler_debug_exception();
void exception_handler_NMI();
void exception_handler_breakpoint();
void exception_handler_overflow();
void exception_handler_bound_range_exceeded();
void exception_handler_invalid_opcode();
void exception_handler_device_unavailable();
void exception_handler_double_fault();
void exception_handler_invalid_tss();
void exception_handler_segment_absent();
void exception_handler_stack_segment_fault();
void exception_handler_general_protection();
void exception_handler_page_fault();
void exception_handler_fpu_error();
void exception_handler_alignment_check();
void exception_handler_machine_check();
void exception_handler_simd_exception();
void exception_handler_virtualization_exception();
void exception_handler_control_protection_exception();

int irq_install(irq_t irq_id, irq_handler_t handler);

void irq_enable_global();
void irq_disable_global();
void irq_enable(irq_t irq_id);
void irq_disable(irq_t irq_id);

void pic_send_eoi(irq_t irq_id);

typedef uint32_t irq_state_t;
irq_state_t irq_protect();
void irq_unprotect(irq_state_t state);

#endif
