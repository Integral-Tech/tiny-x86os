// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "acpi/reboot.h"
#include "comm/cpu_instr.h"

int sys_reboot() {
  uint8_t good = 0x02;
  while (good & 0x02)
    good = inb(0x64);

  outb(0x64, 0xFE);
  return 0;
}
