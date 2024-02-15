// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

__asm__(".code16gcc");

#define LOADER_START_ADDR 0x8000

void boot_entry(void) {
  ((void (*)(void))LOADER_START_ADDR)(); // force to convert address to function pointer
}
