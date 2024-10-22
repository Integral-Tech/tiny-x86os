// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/memory.h"
#include "cpu/mmu.h"
#include "dev/console.h"
#include "tools/klib.h"
#include "tools/log.h"

static addr_alloc_t paddr_alloc;
static pde_t kernel_page_dir[PAGE_DIR_NUM]
    __attribute__((aligned(MEM_PAGE_SIZE)));

static void addr_alloc_init(addr_alloc_t *addr_alloc, uint8_t *bit_arr,
                            uint32_t start, uint32_t size, uint32_t page_size) {
  mutex_init(&addr_alloc->mutex);

  addr_alloc->start = start;
  addr_alloc->size = size;
  addr_alloc->page_size = page_size;

  bmp_init(&addr_alloc->bitmap, bit_arr, size / page_size, 0);
}

static uint32_t addr_alloc_page(addr_alloc_t *addr_alloc, int pages) {
  mutex_lock(&addr_alloc->mutex);

  uint32_t addr = 0;
  const int page_index = bmp_alloc_multi_bit(&addr_alloc->bitmap, 0, pages);

  if (page_index >= 0)
    addr = addr_alloc->start + page_index * addr_alloc->page_size;

  mutex_unlock(&addr_alloc->mutex);
  return addr;
}

static void addr_free_page(addr_alloc_t *addr_alloc, uint32_t addr, int pages) {
  mutex_lock(&addr_alloc->mutex);

  const int page_index = (addr - addr_alloc->start) / addr_alloc->page_size;
  bmp_set_bit(&addr_alloc->bitmap, page_index, pages, 0);

  mutex_unlock(&addr_alloc->mutex);
}

static void show_mem_info(const boot_info_t *boot_info) {
  log_printf("Memory region:");
  for (int i = 0; i < boot_info->ram_regions; i++) {
    log_printf("[%d]: 0x%x - 0x%x", i, boot_info->ram_region_cfg[i].start,
               boot_info->ram_region_cfg[i].start +
                   boot_info->ram_region_cfg[i].size);
  }
}

static uint32_t total_mem_size(const boot_info_t *boot_info) {
  uint32_t mem_size = 0;
  for (int i = 0; i < boot_info->ram_regions; i++)
    mem_size += boot_info->ram_region_cfg[i].size;

  return mem_size;
}

static pte_t *find_pte(pde_t *page_dir_base, uint32_t vaddr, int alloc) {
  pte_t *pte;
  pde_t *pde = page_dir_base + pde_index(vaddr);

  if (pde->present)
    pte = (pte_t *)pde_paddr(pde);
  else {
    if (!alloc)
      return NULL;

    const uint32_t page_paddr = addr_alloc_page(&paddr_alloc, 1);

    if (!page_paddr)
      return NULL;

    pde->value = page_paddr | PDE_P | PDE_W | PDE_U;

    pte = (pte_t *)page_paddr;
    kernel_memset(pte, 0, MEM_PAGE_SIZE);
  }

  return pte + pte_index(vaddr);
}

static int memory_create_map(pde_t *pde, uint32_t vaddr, uint32_t paddr,
                             int num, uint32_t privilege) {
  for (int i = 0; i < num; i++) {
    pte_t *pte = find_pte(pde, vaddr, 1);
    if (pte == NULL) {
      log_printf("Create page table entry failed!");
      return -1;
    }

    ASSERT(pte->present == 0);
    pte->value = paddr | privilege | PTE_P;

    vaddr += MEM_PAGE_SIZE;
    paddr += MEM_PAGE_SIZE;
  }

  return 0;
}

static void create_kernel_table() {
  extern uint8_t kernel_base[], s_text[], e_text[], s_data[];

  static const memory_map_t kernel_map[] = {
      {.vStart = kernel_base,
       .vEnd = s_text,
       .pStart = kernel_base,
       .privilege = PTE_W},

      {.vStart = s_text, .vEnd = e_text, .pStart = s_text, .privilege = 0},

      {.vStart = s_data,
       .vEnd = (void *)MEM_EBDA_START,
       .pStart = s_data,
       .privilege = PTE_W},

      {.vStart = (void *)CONSOLE_VGA_ADDR,
       .vEnd = (void *)CONSOLE_VGA_END,
       .pStart = (void *)CONSOLE_VGA_ADDR,
       .privilege = PTE_W},

      {.vStart = (void *)MEM_EXT_START,
       .vEnd = (void *)MEM_EXT_END,
       .pStart = (void *)MEM_EXT_START,
       .privilege = PTE_W}};

  for (size_t i = 0; i < ARR_SIZE(kernel_map); i++) {
    const memory_map_t *map = kernel_map + i;

    const uint32_t vStart = down2((uint32_t)map->vStart, MEM_PAGE_SIZE);
    const uint32_t vEnd = up2((uint32_t)map->vEnd, MEM_PAGE_SIZE);
    const uint32_t paddr = down2((uint32_t)map->pStart, MEM_PAGE_SIZE);

    const int pages = (vEnd - vStart) / MEM_PAGE_SIZE;

    memory_create_map(kernel_page_dir, vStart, paddr, pages, map->privilege);
  }
}

uint32_t memory_create_uvm() {
  pde_t *pde = (pde_t *)addr_alloc_page(&paddr_alloc, 1);

  if (!pde)
    return 0;

  kernel_memset(pde, 0, MEM_PAGE_SIZE);
  const uint32_t user_pde_start = pde_index(MEM_TASK_BASE);

  for (size_t i = 0; i < user_pde_start; i++)
    (pde + i)->value = kernel_page_dir[i].value;

  return (uint32_t)pde;
}

void memory_init(const boot_info_t *boot_info) {
  extern uint8_t *mem_free_start;
  uint8_t *mem_free = (uint8_t *)&mem_free_start;

  log_printf("Memory initializing...");
  show_mem_info(boot_info);
  uint32_t mem_up1MB_free = total_mem_size(boot_info) - MEM_EXT_START;
  mem_up1MB_free = down2(mem_up1MB_free, MEM_PAGE_SIZE);
  log_printf("Free memory: 0x%x, size: 0x%x", MEM_EXT_START, mem_up1MB_free);

  addr_alloc_init(&paddr_alloc, mem_free, MEM_EXT_START, mem_up1MB_free,
                  MEM_PAGE_SIZE);

  mem_free += bmp_bytes_cnt(paddr_alloc.size / MEM_PAGE_SIZE);

  ASSERT(mem_free < (uint8_t *)MEM_EBDA_START);

  create_kernel_table();
  mmu_set_page_dir((uint32_t)kernel_page_dir);
}

int memory_alloc_for_page_dir(uint32_t pde, uint32_t vaddr, uint32_t size,
                              int privilege) {
  uint32_t curr_vaddr = vaddr;
  const int pages = up2(size, MEM_PAGE_SIZE) / MEM_PAGE_SIZE;

  for (int i = 0; i < pages; i++) {
    const uint32_t paddr = addr_alloc_page(&paddr_alloc, 1);
    if (!paddr) {
      log_printf("Memory allocation failed because of insufficient memory.");
      return 0;
    }

    const int err =
        memory_create_map((pde_t *)pde, curr_vaddr, paddr, 1, privilege);
    if (err < 0) {
      log_printf("Memory allocation failed, error code = %d.", err);
      return 0;
    }

    curr_vaddr += MEM_PAGE_SIZE;
  }
  return 0;
}

int memory_alloc_page_for(uint32_t addr, uint32_t size, int privilege) {
  return memory_alloc_for_page_dir(get_curr_task()->tss.cr3, addr, size,
                                   privilege);
}

uint32_t memory_alloc_page() { return addr_alloc_page(&paddr_alloc, 1); }

void memory_free_page(uint32_t addr) {
  if (addr < MEM_TASK_BASE) // physical address (free page only)
    addr_free_page(&paddr_alloc, addr, 1);
  else { // virtual address (free page & map)
    pte_t *pte = find_pte(curr_page_dir(), addr, 0);
    ASSERT(pte == NULL && pte->present);
    addr_free_page(&paddr_alloc, pte_paddr(pte), 1);
    pte->value = 0;
  }
}

uint32_t memory_copy_uvm(uint32_t page_dir) {
  const uint32_t target_page_dir = memory_create_uvm();
  if (!target_page_dir)
    goto copy_uvm_failed;

  const uint32_t user_pde_start = pde_index(MEM_TASK_BASE);
  const pde_t *pde = (pde_t *)page_dir + user_pde_start;

  for (size_t i = user_pde_start; i < PAGE_DIR_NUM; i++, pde++) {
    if (!pde->present)
      continue;

    const pte_t *pte = (pte_t *)pde_paddr(pde);
    for (size_t j = 0; j < PAGE_TABLE_NUM; j++, pte++) {
      if (!pte->present)
        continue;

      const uint32_t paddr = addr_alloc_page(&paddr_alloc, 1);
      if (!paddr)
        goto copy_uvm_failed;

      const uint32_t vaddr = page_table_vaddr(i, j);
      const int err = memory_create_map((pde_t *)target_page_dir, vaddr, paddr,
                                        1, get_pte_privilege(pte));
      if (err < 0)
        goto copy_uvm_failed;

      kernel_memcpy((void *)paddr, (void *)vaddr, MEM_PAGE_SIZE);
    }
  }

  return target_page_dir;

copy_uvm_failed:
  if (target_page_dir)
    memory_destroy_uvm(target_page_dir);

  return 0;
}

void memory_destroy_uvm(uint32_t page_dir) {
  const uint32_t user_pde_start = pde_index(MEM_TASK_BASE);
  const pde_t *pde = (pde_t *)page_dir + user_pde_start;

  for (int i = user_pde_start; i < PAGE_DIR_NUM; i++, pde++) {
    if (!pde->present)
      continue;

    const pte_t *pte = (pte_t *)pde_paddr(pde);
    for (int j = 0; j < PAGE_TABLE_NUM; j++, pte++) {
      if (!pte->present)
        continue;

      addr_free_page(&paddr_alloc, pte_paddr(pte), 1);
    }
    addr_free_page(&paddr_alloc, (uint32_t)pde_paddr(pde), 1);
  }
  addr_free_page(&paddr_alloc, page_dir, 1);
}

uint32_t memory_get_paddr(uint32_t page_dir, uint32_t vaddr) {
  const pte_t *pte = find_pte((pde_t *)page_dir, vaddr, 0);

  if (!pte)
    return 0;

  return pte_paddr(pte) + (vaddr & (MEM_PAGE_SIZE - 1));
}

int memory_copy_uvm_data(uint32_t dest, uint32_t page_dir, uint32_t src,
                         uint32_t size) {
  while (size > 0) {
    const uint32_t dest_paddr = memory_get_paddr(page_dir, dest);
    if (!dest_paddr)
      return -1;

    const uint32_t offset_in_page = dest_paddr & (MEM_PAGE_SIZE - 1);
    uint32_t curr_size = MEM_PAGE_SIZE - offset_in_page;
    curr_size = min(curr_size, size);

    kernel_memcpy((void *)dest_paddr, (void *)src, curr_size);
    size -= curr_size;
    dest += curr_size;
    src += curr_size;
  }
  return 0;
}

void *sys_sbrk(ptrdiff_t incr) {
  task_t *task = get_curr_task();
  void *pre_heap_end = (void *)task->heap_end;

  ASSERT(incr >= 0);
  if (!incr) {
    log_printf("sbrk(0): end = 0x%x", pre_heap_end);
    return pre_heap_end;
  }

  uint32_t start = task->heap_end;
  const uint32_t end = start + incr;

  const int start_offset = start % MEM_PAGE_SIZE;
  if (start_offset) {
    if (start_offset + incr <= MEM_PAGE_SIZE) {
      task->heap_end = end;
      return pre_heap_end;
    } else {
      const uint32_t curr_size = MEM_PAGE_SIZE - start_offset;
      start += curr_size;
      incr -= curr_size;
    }
  }

  if (incr) {
    const uint32_t curr_size = end - start;
    if (memory_alloc_page_for(start, curr_size, PTE_P | PTE_U | PTE_W) < 0) {
      log_printf("sbrk: Allocate memory failed!");
      return (void *)-1;
    }
  }
#if 0
  log_printf("sbrk(%d): end = 0x%x", pre_incr, end);
#endif
  task->heap_end = end;
  return (void *)pre_heap_end;
}
