#ifndef BOOT_INFO_H
#define BOOT_INFO_H
#define BOOT_RAM_REGION_MAX 10
#define SECTOR_SIZE 512
#define SYS_KERNEL_START_ADDR 1048576
#include "types.h"

typedef struct _boot_info_t {
  struct {
    uint32_t start, size;
  } ram_region_cfg[BOOT_RAM_REGION_MAX];

  int ram_regions;
} boot_info_t;

#endif
