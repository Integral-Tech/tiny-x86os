    .text
    .global _start
    .extern cStart
_start:
    mov %ss, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    jmp cStart
