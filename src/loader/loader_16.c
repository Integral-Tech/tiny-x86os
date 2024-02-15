// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

__asm__(".code16gcc");
#include "loader.h"

boot_info_t boot_info;
static void show_msg(const char *msg) {
  char c;
  while ((c = *msg++) != '\0') {
    __asm__ __volatile__(
        "mov $0xe,%%ah\n\t"  // ah=0xe
        "mov %[ch],%%al\n\t" // put the character to display to register al
        "int $0x10" ::[ch] "r"(c));
  }
}

static void detect_memory() {
  uint32_t contID = 0;
  uint32_t signature, bytes;
  SMAP_entry_t smap_entry;
  show_msg("Try detecting memory:");
  boot_info.ram_regions = 0;
  for (int i = 0; i < BOOT_RAM_REGION_MAX; i++) {
    const SMAP_entry_t *entry = &smap_entry;
    __asm__ __volatile__("int $0x15"
                         : "=a"(signature), "=c"(bytes), "=b"(contID)
                         : "a"(0xE820), "b"(contID), "c"(24), "d"(0x534D4150),
                           "D"(entry));

    if (signature != 0x534D4150) {
      show_msg("Failed\n\r");
      return;
    }

    if (bytes > 20 && !(entry->ACPI & 0x0001))
      continue;

    if (entry->Type == 1) {
      boot_info.ram_region_cfg[boot_info.ram_regions].start = entry->BaseL;
      boot_info.ram_region_cfg[boot_info.ram_regions].size = entry->LengthL;
      boot_info.ram_regions++;
    }

    if (!contID)
      break;
  }
  show_msg("Detecting finished.\r\n");
}

static const uint16_t gdt_table[][4] = {
    {0, 0, 0, 0},
    {0xFFFF, 0x0000, 0x9a00, 0x00cf},
    {0xFFFF, 0x0000, 0x9200, 0x00cf},
};

static void enter_protected_mode() {
  cli(); // disable cpu interrupt

  const uint8_t v = inb(0x92);
  outb(0x92, v | 0x2);                          // enable the A20 line
  lgdt((uint32_t)gdt_table, sizeof(gdt_table)); // Load GDT
  const uint32_t cr0 = read_cr0();
  write_cr0(cr0 | 0x1); // set the PE digit(the lowest digit) of CR0 to 1
  far_jump(8, (uint32_t)protected_mode_entry);
  // make a far jump to clear the pipeline
}

void loader_entry() {
  show_msg("Loading...\r\n");
  detect_memory();
  enter_protected_mode();
  while (1)
    ;
}
