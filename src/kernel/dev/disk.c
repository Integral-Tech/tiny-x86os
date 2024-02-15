// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dev/disk.h"
#include "comm/boot_info.h"
#include "comm/cpu_instr.h"
#include "tools/klib.h"
#include "tools/log.h"

static disk_t disk_buf[DISK_NUM];
static mutex_t rw_mutex;
static sem_t rw_sem;

static _Bool disk_on_task = FALSE;

const dev_desc_t disk_desc = {.name = "tty",
                              .major_no = DEV_DISK,
                              .open = disk_open,
                              .close = disk_close,
                              .read = disk_read,
                              .write = disk_write,
                              .control = disk_control};

static void disk_send_cmd(const disk_t *disk, uint32_t start_sector,
                          uint32_t sectors, int cmd) {
  outb(DRIVE_REG(disk), DRIVE_REG_BASE | disk->drive_type);
  outb(SECTOR_CNT_REG(disk), (uint8_t)(sectors >> 8));
  outb(LBA_LO_REG(disk), (uint8_t)(start_sector >> 24)); // LBA4
  outb(LBA_MID_REG(disk), 0);                            // LBA5
  outb(LBA_HI_REG(disk), 0);                             // LBA6

  outb(SECTOR_CNT_REG(disk), (uint8_t)sectors);
  outb(LBA_LO_REG(disk), (uint8_t)start_sector);         // LBA1
  outb(LBA_MID_REG(disk), (uint8_t)(start_sector >> 8)); // LBA2
  outb(LBA_HI_REG(disk), (uint8_t)(start_sector >> 16)); // LBA3

  outb(CMD_REG(disk), cmd);
}

static void read_disk(const disk_t *disk, void *buf, size_t size) {
  uint16_t *dest = buf;
  for (size_t i = 0; i < size / 2; i++)
    *dest++ = inw(DATA_REG(disk));
}

static void write_disk(const disk_t *disk, const void *buf, size_t size) {
  const uint16_t *src = buf;
  for (size_t i = 0; i < size / 2; i++)
    outw(DATA_REG(disk), *src++);
}

static int disk_wait_data(const disk_t *disk) {
  uint8_t status;
  do {
    status = inb(STATUS_REG(disk));
  } while ((status & (STATUS_BSY | STATUS_DRQ | STATUS_ERR)) == STATUS_BSY);

  return (status & STATUS_ERR) ? -1 : 0;
}

static int detect_part_info(disk_t *disk) {
  mbr_t mbr;

  disk_send_cmd(disk, 0, 1, CMD_READ);
  const int err = disk_wait_data(disk);
  if (err < 0) {
    log_printf("Failed to read MBR!");
    return err;
  }

  read_disk(disk, &mbr, sizeof(mbr_t));
  part_entry_t *part_entry = mbr.part_table;
  part_info_t *part_info = disk->part_info + 1;
  // The first partition represents the entire disk

  for (int i = 0; i < MBR_PRIMARY_PARTC; i++, part_entry++, part_info++) {
    part_info->type = part_entry->system_id;
    if (part_info->type == FS_INVALID)
      kernel_memset(part_info, 0, sizeof(part_info_t));
    else {
      kernel_sprintf(part_info->name, "%s%d", disk->name, i + 1);
      part_info->start_sector = part_entry->relative_sector;
      part_info->total_sector = part_entry->total_sector;
      part_info->disk = disk;
    }
  }

  return 0;
}

static int identify_disk(disk_t *disk) {
  disk_send_cmd(disk, 0, 0, CMD_IDENTIFY);

  if (!inb(STATUS_REG(disk))) {
    log_printf("No such disk: %s", disk->name);
    return -1;
  }

  const int err = disk_wait_data(disk);
  if (err < 0) {
    log_printf("Failed to read data from disk %s!", disk->name);
    return err;
  }

  uint16_t disk_buf[DISK_BUF_SIZE];
  read_disk(disk, disk_buf, sizeof(disk_buf));
  disk->sectors = sectors(disk_buf);
  disk->sector_size = SECTOR_SIZE;

  /*
   * Regard the entire disk as a huge partition,
   * and save its information to the 0th item of the partition table
   */
  part_info_t *part = disk->part_info;
  part->disk = disk;
  kernel_sprintf(part->name, "%s%d", disk->name, 0);
  part->start_sector = 0;
  part->total_sector = disk->sectors;
  part->type = FS_INVALID;

  detect_part_info(disk);
  return 0;
}

static void print_disk_info(disk_t *disk) {
  log_printf("%s", disk->name);
  log_printf("Base Address of Port: 0x%x", disk->port_base);
  log_printf("Total size: %d MiB", disk_size_mib(disk));

  for (int i = 0; i < PRIMARY_PART_NUM; i++) {
    part_info_t *part_info = disk->part_info + i;
    if (part_info->type != FS_INVALID) {
      log_printf("%s: Type = %x, Starting sector = %d, Total sectors: %d",
                 part_info->name, part_info->type, part_info->start_sector,
                 part_info->total_sector);
    }
  }
}

void disk_init() {
  log_printf("Initializing disk...");

  mutex_init(&rw_mutex);
  sem_init(&rw_sem, 0);
  kernel_memset(disk_buf, 0, sizeof(disk_buf));
  for (int i = 0; i < DISK_PER_BUS; i++) {
    disk_t *disk = disk_buf + i;
    kernel_sprintf(disk->name, "Disk: /dev/hd%c", i + 'a');
    disk->drive_type = (i == 0) ? MASTER : SLAVE;
    disk->port_base = PRIMARY_BUS_BASE;
    disk->rw_mutex = &rw_mutex;
    disk->rw_sem = &rw_sem;

    if (identify_disk(disk) < 0)
      log_printf("Failed to identify disk %s!", disk->name);

    print_disk_info(disk);
  }
}

int disk_open(device_t *dev) {
  const int disk_id = get_disk_id(dev->minor_no);
  const int part_id = get_part_id(dev->minor_no);

  if (disk_id >= DISK_NUM || part_id >= PRIMARY_PART_NUM) {
    log_printf("Invalid minor number!");
    return -1;
  }

  const disk_t *disk = disk_buf + disk_id;
  if (!disk->sectors) {
    log_printf("Disk does not exist: /dev/hd%x", dev->minor_no);
    return -1;
  }

  const part_info_t *part_info = disk->part_info + part_id;
  if (!part_info->total_sector) {
    log_printf("Partition does not exist: /dev/hd%x", dev->minor_no);
    return -1;
  }

  dev->data = (void *)part_info;

  irq_install(IRQ14_IDE_PRIMARY, (irq_handler_t)exception_handler_ide_primary);
  irq_enable(IRQ14_IDE_PRIMARY);
  return 0;
}

int disk_close(const device_t *dev) { return -1; }

int disk_read(const device_t *dev, uint32_t start_sector, void *buf,
              size_t sectors) {
  const part_info_t *part_info = dev->data;
  if (!part_info) {
    log_printf("Failed to get partition information of device %d!",
               dev->minor_no);
    return -1;
  }

  const disk_t *disk = part_info->disk;
  if (!disk) {
    log_printf("Disk %d does not exist!", dev->minor_no);
    return -1;
  }

  mutex_lock(disk->rw_mutex);
  disk_on_task = TRUE;
  disk_send_cmd(disk, part_info->start_sector + start_sector, sectors,
                CMD_READ);

  size_t sector_read;
  uint8_t *dest = buf;

  for (sector_read = 0; sector_read < sectors; sector_read++) {
    if (get_curr_task())
      sem_wait(disk->rw_sem);

    const int err = disk_wait_data(disk);
    if (err < 0) {
      log_printf("An error occured while reading disk %s!", disk->name);
      log_printf("Starting sector: %d, Number of sectors: %d", start_sector,
                 sectors);
      break;
    }

    read_disk(disk, dest, disk->sector_size);
    dest += disk->sector_size;
  }

  mutex_unlock(disk->rw_mutex);
  return sector_read;
}

int disk_write(const device_t *dev, uint32_t start_sector, const void *buf,
               size_t sectors) {
  if (!sectors)
    return -1;

  const part_info_t *part_info = dev->data;
  if (!part_info) {
    log_printf("Failed to get partition information of device %d!",
               dev->minor_no);
    return -1;
  }

  const disk_t *disk = part_info->disk;
  if (!disk) {
    log_printf("Disk %d does not exist!", dev->minor_no);
    return -1;
  }

  mutex_lock(disk->rw_mutex);
  disk_on_task = TRUE;
  disk_send_cmd(disk, part_info->start_sector + start_sector, sectors,
                CMD_WRITE);

  size_t sector_written = 0;
  const uint8_t *dest = buf;
  do {
    write_disk(disk, dest, disk->sector_size);

    if (get_curr_task())
      sem_wait(disk->rw_sem);

    const int err = disk_wait_data(disk);
    if (err < 0) {
      log_printf("An error occured while writing disk %s!", disk->name);
      log_printf("Starting sector: %d, Number of sectors: %d", start_sector,
                 sectors);
      break;
    }

    dest += disk->sector_size;
  } while (++sector_written < sectors);

  mutex_unlock(disk->rw_mutex);
  return sector_written;
}

int disk_control(const device_t *dev, int cmd, va_list arg_list) { return -1; }

void do_handle_ide_primary(exception_frame_t *frame) {
  pic_send_eoi(IRQ14_IDE_PRIMARY);
  if (disk_on_task && get_curr_task())
    sem_notify(&rw_sem);
}
