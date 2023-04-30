#ifndef DISK_H
#define DISK_H

#include "cpu/irq.h"
#include "dev/dev.h"
#include "ipc/mutex.h"
#include "ipc/sem.h"

#define DISK_NAME_SIZE 32
#define PART_NAME_SIZE 32
#define PRIMARY_PART_NUM (4 + 1)

#define DISK_NUM 2

#define DISK_PER_BUS 2

#define PRIMARY_BUS_BASE 0x1F0
#define SECONDARY_BUS_BASE 0x170

#define DATA_REG(disk) (((disk)->port_base) + 0)
#define ERR_REG(disk) (((disk)->port_base) + 1)
#define FEATURE_REG(disk) (((disk)->port_base) + 1)
#define SECTOR_CNT_REG(disk) (((disk)->port_base) + 2)
#define LBA_LO_REG(disk) (((disk)->port_base) + 3)
#define LBA_MID_REG(disk) (((disk)->port_base) + 4)
#define LBA_HI_REG(disk) (((disk)->port_base) + 5)
#define DRIVE_REG(disk) (((disk)->port_base) + 6)
#define STATUS_REG(disk) (((disk)->port_base) + 7)
#define CMD_REG(disk) (((disk)->port_base) + 7)

#define DRIVE_REG_BASE 0xE0

#define disk_size_mib(disk)                                                    \
  (((disk)->sectors) * ((disk)->sector_size) / 1024 / 1024)
// Unit: MiB

#define sectors(disk_buf) (*(uint32_t *)((disk_buf) + 100))

#define get_disk_id(minor_no) (((minor_no) >> 4) - 0xa)
#define get_part_id(minor_no) ((minor_no) & 0xF)

#define DISK_BUF_SIZE 256

#define MBR_PRIMARY_PARTC 4

enum disk_status_t {
  STATUS_ERR = (1 << 0),
  STATUS_IDX = (1 << 1),
  STATUS_CORR = (1 << 2),
  STATUS_DRQ = (1 << 3),
  STATUS_SRV = (1 << 4),
  STATUS_DF = (1 << 5),
  STATUS_RDY = (1 << 6),
  STATUS_BSY = (1 << 7)
};

enum disk_cmd_t { CMD_READ = 0x24, CMD_WRITE = 0x34, CMD_IDENTIFY = 0xEC };

#pragma pack(1)

typedef struct _part_entry_t {
  uint8_t bootable;
  uint8_t start_head;
  uint16_t start_sector : 6;
  uint16_t start_cylinder : 10;
  uint8_t system_id;
  uint8_t end_head;
  uint16_t end_sector : 6;
  uint16_t end_cylinder : 10;
  uint32_t relative_sector;
  uint32_t total_sector;
} part_entry_t;

typedef struct _mbr_t {
  uint8_t bootstrap_code[446];
  part_entry_t part_table[MBR_PRIMARY_PARTC];
  const uint16_t boot_sign; // 0x55AA
} mbr_t;

#pragma pack()

struct _disk_t;

typedef struct _part_info_t {
  char name[PART_NAME_SIZE];
  const struct _disk_t *disk;
  unsigned int start_sector, total_sector;
  enum { FS_INVALID, FS_FAT16_DOS = 0x6, FS_FAT16_WIN95 = 0xE } type;
} part_info_t;

typedef struct _disk_t {
  char name[DISK_NAME_SIZE];
  enum { MASTER = (0 << 4), SLAVE = (1 << 4) } drive_type;
  uint16_t port_base;
  size_t sector_size, sectors;
  part_info_t part_info[PRIMARY_PART_NUM];

  mutex_t *rw_mutex;
  sem_t *rw_sem;
} disk_t;

void disk_init();
int disk_open(device_t *dev);
int disk_close(const device_t *dev);
int disk_read(const device_t *dev, uint32_t start_sector, void *buf,
              size_t sectors);
int disk_write(const device_t *dev, uint32_t start_sector, const void *buf,
               size_t sectors);
int disk_control(const device_t *dev, int cmd, va_list arg_list);
void exception_handler_ide_primary();
void do_handle_ide_primary(exception_frame_t *frame);

#endif
