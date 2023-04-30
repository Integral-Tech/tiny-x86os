#include "cpu/irq.h"
#include "comm/cpu_instr.h"
#include "os_cfg.h"
#include "tools/log.h"

static gate_desc_t idt_table[IDT_TABLE_NUM];

void dump_core_regs(const exception_frame_t *frame) {
  uint32_t ss, esp;

  if (frame->cs & SEG_CPL3) {
    ss = frame->ss3;
    esp = frame->esp3;
  } else {
    ss = frame->ds;
    esp = frame->esp;
  }

  log_printf("Interrupt number: %d  error code: %d", frame->num,
             frame->err_code);
  log_printf("EAX: 0x%x  EBX: 0x%x  ECX: 0x%x  EDX: 0x%x", frame->eax,
             frame->ebx, frame->ecx, frame->edx);
  log_printf("ESP: 0x%x  EBP: 0x%x  ESI: 0x%x  EDI: 0x%x", esp, frame->ebp,
             frame->esi, frame->edi);
  log_printf("CS: 0x%x  DS: 0x%x  ES: 0x%x  SS: 0x%x", frame->cs, frame->ds,
             frame->es, ss);
  log_printf("FS: 0x%x  GS: 0x%x  EIP: 0x%x  EFLAGS: 0x%x", frame->fs,
             frame->gs, frame->eip, frame->eflags);
}

static void handler_end(const exception_frame_t *frame) {
  if (frame->cs & SEG_CPL3) // Application (CPL=3)
    sys_exit(frame->err_code);
  else // System (CPL=0)
    hlt();
}

static void do_default_handler(const exception_frame_t *frame,
                               const char *message) {
  log_printf("Interrupt/Exception occured: %s", message);
  dump_core_regs(frame);

  handler_end(frame);
}

void do_handle_unknown(const exception_frame_t *frame) {
  do_default_handler(frame, "Unknown Exception");
}

void do_handle_divide_error(const exception_frame_t *frame) {
  do_default_handler(frame, "Divide Error");
}

void do_handle_debug_exception(const exception_frame_t *frame) {
  do_default_handler(frame, "Debug Exception");
}

void do_handle_NMI(const exception_frame_t *frame) {
  do_default_handler(frame, "NMI Interrupt");
}

void do_handle_breakpoint(const exception_frame_t *frame) {
  do_default_handler(frame, "Breakpoint");
}

void do_handle_overflow(const exception_frame_t *frame) {
  do_default_handler(frame, "Overflow");
}

void do_handle_bound_range_exceeded(const exception_frame_t *frame) {
  do_default_handler(frame, "Bound Range Exceeded");
}

void do_handle_invalid_opcode(const exception_frame_t *frame) {
  do_default_handler(frame, "Invalid Opcode");
}

void do_handle_device_unavailable(const exception_frame_t *frame) {
  do_default_handler(frame, "Device Not Available");
}

void do_handle_double_fault(const exception_frame_t *frame) {
  do_default_handler(frame, "Double Fault");
}

void do_handle_invalid_tss(const exception_frame_t *frame) {
  do_default_handler(frame, "Invalid TSS");
}

void do_handle_segment_absent(const exception_frame_t *frame) {
  do_default_handler(frame, "Segment Not Present");
}

void do_handle_stack_segment_fault(const exception_frame_t *frame) {
  do_default_handler(frame, "Stack-Segment Fault");
}

void do_handle_general_protection(const exception_frame_t *frame) {
  log_printf("General Protection Fault occured!");

  const uint32_t cr2 = read_cr2();
  if (frame->err_code & ERR_EXT) {
    log_printf("The exception occurred during delivery of an event external to "
               "the program: 0x%x",
               cr2);
  } else {
    log_printf(
        "The exception occurred during delivery of a software interrupt: 0x%x",
        cr2);
  }

  if (frame->err_code & ERR_IDT) {
    log_printf("The index portion of the error code refers to a gate "
               "descriptor in the IDT: 0x%x",
               cr2);
  } else {
    log_printf(
        "The index refers to a descriptor in the GDT or the current LDT: 0x%x",
        cr2);
  }

  log_printf("Selector index: %d", frame->err_code & 0xFFF8);
  dump_core_regs(frame);
  handler_end(frame);
}

void do_handle_page_fault(const exception_frame_t *frame) {
  log_printf("Page Fault occured!");

  const uint32_t cr2 = read_cr2();
  if (frame->err_code & ERR_PAGE_P)
    log_printf(
        "The fault was caused by a page-level protection violation: 0x%x", cr2);
  else
    log_printf("The fault was caused by a non-present page: 0x%x", cr2);

  if (frame->err_code & ERR_PAGE_WR)
    log_printf("The access causing the fault was a write: 0x%x", cr2);
  else
    log_printf("The access causing the fault was a read: 0x%x", cr2);

  if (frame->err_code & ERR_PAGE_US)
    log_printf("A user-mode access caused the fault: 0x%x", cr2);
  else
    log_printf("A supervisor-mode access caused the fault: 0x%x", cr2);

  dump_core_regs(frame);
  handler_end(frame);
}

void do_handle_fpu_error(const exception_frame_t *frame) {
  do_default_handler(frame, "X87 FPU Floating Point Error");
}

void do_handle_alignment_check(const exception_frame_t *frame) {
  do_default_handler(frame, "Alignment Check");
}

void do_handle_machine_check(const exception_frame_t *frame) {
  do_default_handler(frame, "Machine Check");
}

void do_handle_simd_exception(const exception_frame_t *frame) {
  do_default_handler(frame, "SIMD Floating Point Exception");
}

void do_handle_virtualization_exception(const exception_frame_t *frame) {
  do_default_handler(frame, "Virtualization Exception");
}

void do_handle_control_protection_exception(const exception_frame_t *frame) {
  do_default_handler(frame, "Control Protection Exception");
}

static void init_pic() {
  // ICW4 Needed, Cascade Mode, Edge Triggered Mode
  outb(PIC0_ICW1, PIC_ICW1_ALWAYS_1 | PIC_ICW1_ICW4);
  outb(PIC0_ICW2, IRQ_PIC_START);
  outb(PIC0_ICW3, 1 << 2); // IR2 is connected
  outb(PIC0_ICW4, PIC_ICW4_8086);

  outb(PIC1_ICW1, PIC_ICW1_ALWAYS_1 | PIC_ICW1_ICW4);
  outb(PIC1_ICW2, IRQ_PIC_START + 8);
  outb(PIC1_ICW3, 2); // ICW3 is connected to IR2
  outb(PIC1_ICW4, PIC_ICW4_8086);

  outb(PIC0_IMR, 0xFF & ~(1 << 2));
  outb(PIC1_IMR, 0xFF);
}

void irq_init() {
  for (int i = 0; i < IDT_TABLE_NUM; i++) {
    gate_desc_set(idt_table + i, KERNEL_SELECTOR_CS,
                  (uint32_t)exception_handler_unknown,
                  GATE_P | GATE_DPL0 | GATE_TYPE_INT);
  }

  irq_install(IRQ0_DE, (irq_handler_t)exception_handler_divide_error);
  irq_install(IRQ1_DB, (irq_handler_t)exception_handler_debug_exception);
  irq_install(IRQ2_NMI, (irq_handler_t)exception_handler_NMI);
  irq_install(IRQ3_BP, (irq_handler_t)exception_handler_breakpoint);
  irq_install(IRQ4_OF, (irq_handler_t)exception_handler_overflow);
  irq_install(IRQ5_BR, (irq_handler_t)exception_handler_bound_range_exceeded);
  irq_install(IRQ6_UD, (irq_handler_t)exception_handler_invalid_opcode);
  irq_install(IRQ7_NM, (irq_handler_t)exception_handler_device_unavailable);
  irq_install(IRQ8_DF, (irq_handler_t)exception_handler_double_fault);
  irq_install(IRQ10_TS, (irq_handler_t)exception_handler_invalid_tss);
  irq_install(IRQ11_NP, (irq_handler_t)exception_handler_segment_absent);
  irq_install(IRQ12_SS, (irq_handler_t)exception_handler_stack_segment_fault);
  irq_install(IRQ13_GP, (irq_handler_t)exception_handler_general_protection);
  irq_install(IRQ14_PF, (irq_handler_t)exception_handler_page_fault);
  irq_install(IRQ16_MF, (irq_handler_t)exception_handler_fpu_error);
  irq_install(IRQ17_AC, (irq_handler_t)exception_handler_alignment_check);
  irq_install(IRQ18_MC, (irq_handler_t)exception_handler_machine_check);
  irq_install(IRQ19_XM, (irq_handler_t)exception_handler_simd_exception);
  irq_install(IRQ20_VE,
              (irq_handler_t)exception_handler_virtualization_exception);
  irq_install(IRQ21_CP,
              (irq_handler_t)exception_handler_control_protection_exception);
  lidt((uint32_t)idt_table, sizeof(idt_table));

  init_pic();
}

int irq_install(irq_t irq_id, irq_handler_t handler) {
  if (irq_id > IDT_TABLE_NUM)
    return -1;

  gate_desc_set(idt_table + irq_id, KERNEL_SELECTOR_CS, (uint32_t)handler,
                GATE_P | GATE_DPL0 | GATE_TYPE_INT);
  return 0;
}

void irq_enable_global() { sti(); }

void irq_disable_global() { cli(); }

void irq_enable(irq_t irq_id) {
  if (irq_id < IRQ_PIC_START)
    return;
  // judge if the irq_id is valid

  irq_id -= IRQ_PIC_START;
  if (irq_id < 8) {
    const uint8_t mask = inb(PIC0_IMR) & ~(1 << irq_id);
    outb(PIC0_IMR, mask);
  } // the master 8259 chip
  else {
    irq_id -= 8;
    const uint8_t mask = inb(PIC1_IMR) & ~(1 << irq_id);
    outb(PIC1_IMR, mask);
  } // the slave 8259 chip
}

void irq_disable(irq_t irq_id) {
  if (irq_id < IRQ_PIC_START)
    return;
  // judge if the irq_id is valid

  irq_id -= IRQ_PIC_START;
  if (irq_id < 8) {
    const uint8_t mask = inb(PIC0_IMR) | (1 << irq_id);
    outb(PIC0_IMR, mask);
  } // the master 8259 chip
  else {
    irq_id -= 8;
    const uint8_t mask = inb(PIC1_IMR) | (1 << irq_id);
    outb(PIC1_IMR, mask);
  } // the slave 8259 chip
}

void pic_send_eoi(irq_t irq_id) {
  irq_id -= IRQ_PIC_START;
  if (irq_id >= 8)
    outb(PIC1_OCW2, PIC_OCW2_EOI);

  outb(PIC0_OCW2, PIC_OCW2_EOI);
}

/*
 * Ensure IF unchanged after entering and leaving protection area,
 * save eflags to the variable "state",
 * and then restore eflags from the variable "state".
 */
irq_state_t irq_protect() {
  const irq_state_t state = read_eflags();
  irq_disable_global();
  return state;
}

void irq_unprotect(irq_state_t state) { write_eflags(state); }
