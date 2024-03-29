// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "os_cfg.h"
    .text //code segment
    .extern kernel_init
    .extern init_main
    .global _start

_start:
    mov 0x4(%esp), %eax
    push %eax
    call kernel_init // kernel_init(boot_info)

    jmp $KERNEL_SELECTOR_CS, $gdt_reload
gdt_reload:
    mov $KERNEL_SELECTOR_DS, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss
    mov %ax, %fs
    mov %ax, %gs

    mov $(stack+KERNEL_STACK_SIZE),%esp
    jmp init_main

    .bss
.comm stack, KERNEL_STACK_SIZE

    .text
.macro exception_handler name num with_err_code
    .extern do_handle_\name
    .global exception_handler_\name
exception_handler_\name:
    .if \with_err_code == 0
        push $0
    .endif

    push $\num
    pusha //push eax, ebx, ecx, edx, esp, ebp, edi, esi
    push %ds
    push %es
    push %fs
    push %gs

    push %esp
    call do_handle_\name
    add $4, %esp

    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa
    add $8, %esp
    iret
.endm

exception_handler unknown, -1, 0
exception_handler divide_error, 0, 0
exception_handler debug_exception, 1, 0
exception_handler NMI, 2, 0
exception_handler breakpoint, 3, 0
exception_handler overflow, 4, 0
exception_handler bound_range_exceeded, 5, 0
exception_handler invalid_opcode, 6, 0
exception_handler device_unavailable, 7, 0
exception_handler double_fault, 8, 1
exception_handler invalid_tss, 10, 1
exception_handler segment_absent, 11, 1
exception_handler stack_segment_fault, 12, 1
exception_handler general_protection, 13, 1
exception_handler page_fault, 14, 1
exception_handler fpu_error, 16, 0
exception_handler alignment_check, 17, 1
exception_handler machine_check, 18, 0
exception_handler simd_exception, 19, 0
exception_handler virtualization_exception, 20, 0
exception_handler control_protection_exception, 21, 1
exception_handler time, 0x20, 0
exception_handler keyboard, 0x21, 0
exception_handler ide_primary, 0x2E, 0

    //simple_switch(&from, to)
    .text
    .global simple_switch
simple_switch:
    mov 4(%esp), %eax //feteh argument "from"
    mov 8(%esp), %edx //fetch argument "to"
    push %ebx
    push %ebp
    push %esi
    push %edi
    mov %esp, (%eax) //save current stack

    mov %edx, %esp //switch stack
    pop %edi
    pop %esi
    pop %ebp
    pop %ebx
    ret

    .extern do_handle_syscall
    .global syscall_handler
syscall_handler:
    pusha
    push %ds
    push %es
    push %fs
    push %gs
    pushf

    mov %esp, %eax
    push %eax // a pointer to the struct "syscall_frame_t"

    call do_handle_syscall
    add $4, %esp

    popf
    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa

    retf $(5*4) // 5 arguments
