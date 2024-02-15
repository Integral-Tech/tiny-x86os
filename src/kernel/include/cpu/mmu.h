// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MMU_H
#define MMU_H

#include "comm/cpu_instr.h"

#define PAGE_DIR_NUM 1024
#define PAGE_TABLE_NUM 1024
#define PTE_P (1 << 0)
#define PDE_P (1 << 0)
#define PTE_W (1 << 1)
#define PDE_W (1 << 1)
#define PTE_U (1 << 2)
#define PDE_U (1 << 2)

#define PDE_RW (1 << 1)
#define PDE_PS (1 << 7) // PS bit = 1 -> Page Size = 4MB

#define CR4_PSE (1 << 4)
#define CR0_PG (1 << 31)

#define mmu_set_page_dir(paddr) write_cr3(paddr)

#define pde_index(vaddr) ((vaddr) >> 22)
#define pte_index(vaddr) ((vaddr) >> 12 & 0x3FF)

#define pde_paddr(pde) (((pde)->physical_addr) << 12)
#define pte_paddr(pte) (((pte)->physical_addr) << 12)

#define page_table_vaddr(pde_index, pte_index)                                 \
  (((pde_index) << 22) | ((pte_index) << 12))

#define get_pte_privilege(pte) ((pte)->value & 0x3FF)

typedef union _pde_t {
  uint32_t value;
  struct {
    uint32_t present : 1;
    uint32_t write_allowed : 1;
    uint32_t user_mode_allowed : 1;
    uint32_t write_through : 1;
    uint32_t cache_disable : 1;
    uint32_t accessed : 1;
    uint32_t : 1;
    uint32_t ps : 1;
    uint32_t : 4;
    uint32_t physical_addr : 20;
  };
} pde_t; // Page Directory Entry

typedef union _pte_t {
  uint32_t value;
  struct {
    uint32_t present : 1;
    uint32_t write_allowed : 1;
    uint32_t user_mode_allowed : 1;
    uint32_t write_through : 1;
    uint32_t cache_disable : 1;
    uint32_t accessed : 1;
    uint32_t dirty : 1;
    uint32_t pat : 1;
    uint32_t global : 1;
    uint32_t : 3;
    uint32_t physical_addr : 20;
  };
} pte_t; // Page Table Entry

#endif
