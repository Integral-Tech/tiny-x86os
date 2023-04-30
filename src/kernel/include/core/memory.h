#ifndef MEMORY_H
#define MEMORY_H

#include "comm/boot_info.h"
#include "ipc/mutex.h"
#include "tools/bitmap.h"

#define MEM_EXT_START 1048576
#define MEM_EXT_END (127 * 1024 * 1024)
#define MEM_PAGE_SIZE 4096
#define MEM_EBDA_START 0x80000
#define MEM_TASK_BASE 0x80000000

#define MEM_TASK_STACK_TOP 0xE0000000
#define MEM_TASK_STACK_SIZE (MEM_PAGE_SIZE * 500)
#define MEM_TASK_ARG_SIZE (MEM_PAGE_SIZE * 4)

typedef struct _addr_alloc_t {
  mutex_t mutex;
  bitmap_t bitmap;

  uint32_t start, size, page_size;
} addr_alloc_t;

typedef struct _memory_map_t {
  void *vStart, *vEnd, *pStart;
  uint32_t privilege;
} memory_map_t;

#define curr_page_dir() ((pde_t *)(((get_curr_task())->tss).cr3))

void memory_init(const boot_info_t *boot_info);
uint32_t memory_create_uvm();
int memory_alloc_for_page_dir(uint32_t pde, uint32_t vaddr, uint32_t size,
                              int privilege);
int memory_alloc_page_for(uint32_t addr, uint32_t size, int privilege);
uint32_t memory_alloc_page();
void memory_free_page(uint32_t addr);
void memory_destroy_uvm(uint32_t page_dir);
uint32_t memory_copy_uvm(uint32_t page_dir);
uint32_t memory_get_paddr(uint32_t page_dir, uint32_t vaddr);
int memory_copy_uvm_data(uint32_t dest, uint32_t page_dir, uint32_t src,
                         uint32_t size);

void *sys_sbrk(ptrdiff_t incr);

#endif
