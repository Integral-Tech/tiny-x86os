// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "comm/elf.h"
#include "core/memory.h"
#include "cpu/mmu.h"
#include "loader.h"

static void read_disk(uint32_t sector, uint32_t sectors, const uint8_t *buf) {
  outb(0x1F6, 0xE0); // LBA mode enabled, master hard drive enabled
  outb(0x1F2,
       (uint8_t)(sectors >> 8));        // the higher 8 bytes of sectors
  outb(0x1F3, (uint8_t)(sector >> 24)); // LBA4
  outb(0x1F4, 0);                       // LBA5
  outb(0x1F5, 0);                       // LBA6

  outb(0x1F2, (uint8_t)sectors);        // the lower 8 bytes of sectors
  outb(0x1F3, (uint8_t)sector);         // LBA1
  outb(0x1F4, (uint8_t)(sector >> 8));  // LBA2
  outb(0x1F5, (uint8_t)(sector >> 16)); // LBA3

  outb(0x1F7, 0x24); // send the "READ SECTORS EXT" command(0x24) to port 0x1F7

  uint16_t *data_buf = (uint16_t *)buf;
  while (sectors--) {
    while ((inb(0x1F7) & 0x88) != 0x8)
      ; // wait until status register DRQ=1, BSY=0

    for (int i = 0; i < SECTOR_SIZE / 2; i++)
      *data_buf++ = inw(0x1F0); // read from data register to data_buf
  }
}

static uint32_t reload_elf_file(const uint8_t *file_buf) {
  const Elf32_Ehdr *elf_hdr = (Elf32_Ehdr *)file_buf;
  if (elf_hdr->e_ident[0] != ELF_MAGIC || elf_hdr->e_ident[1] != 'E' ||
      elf_hdr->e_ident[2] != 'L' || elf_hdr->e_ident[3] != 'F') {
    return 0;
  } // verify the elf_file

  for (int i = 0; i < elf_hdr->e_phnum; i++) {
    const Elf32_Phdr *phdr = (Elf32_Phdr *)(file_buf + elf_hdr->e_phoff) + i;
    if (phdr->p_type != PT_LOAD)
      continue; // the program header cannot be loaded

    const uint8_t *src = file_buf + phdr->p_offset;
    uint8_t *dest = (uint8_t *)phdr->p_paddr;

    for (size_t j = 0; j < phdr->p_filesz; j++)
      *dest++ = *src++; // do copying

    dest = (uint8_t *)phdr->p_paddr + phdr->p_filesz;
    for (size_t j = 0; j < phdr->p_memsz - phdr->p_filesz; j++)
      *dest++ = 0; // set .bss section to 0
  }

  return elf_hdr->e_entry;
  // return the address of entry
}

static void panic() {
  while (1)
    hlt();
}

void enable_paging() {
  static uint32_t page_dir[1024]
      __attribute__((aligned(MEM_PAGE_SIZE))) = {[0] = PDE_P | PDE_RW | PDE_PS};

  const uint32_t cr4 = read_cr4();
  write_cr4(cr4 | CR4_PSE);

  write_cr3((uint32_t)page_dir);

  const uint32_t cr0 = read_cr0();
  write_cr0(cr0 | CR0_PG);
}

void load_kernel() {
  // read from sector 100 and read 500 sectors
  read_disk(100, 500, (uint8_t *)SYS_KERNEL_START_ADDR);
  const uint32_t kernel_entry =
      reload_elf_file((uint8_t *)SYS_KERNEL_START_ADDR);
  if (!kernel_entry)
    panic(); // verify the kernel_entry

  enable_paging();
  ((void (*)(boot_info_t *))kernel_entry)(&boot_info); // jump into the kernel
}
