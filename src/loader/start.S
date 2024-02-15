// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

.code16
 	.text
	.global _start
	.extern loader_entry 
_start:
	jmp loader_entry

	.code32
	.text
	.global protected_mode_entry
	.extern load_kernel
protected_mode_entry:
	mov $16,%ax
	mov %ax,%ds
	mov %ax,%ss
	mov %ax,%es
	mov %ax,%fs
	mov %ax,%gs /* initialize segment registers to 0 */
	jmp $8, $load_kernel

	jmp .
