#include "dev/dev.h"
#include "cpu/irq.h"
#include "tools/klib.h"

extern dev_desc_t tty_desc;
extern dev_desc_t disk_desc;

/*
 * dev_desc_table is for different device types
 * dev_table is for specific devices
 */
static dev_desc_t *dev_desc_table[] = {&tty_desc, &disk_desc};
static device_t dev_table[DEV_TABLE_SIZE];

static _Bool is_dev_id_valid(int dev_id) {
  if (dev_id < 0 || dev_id >= DEV_TABLE_SIZE || dev_table[dev_id].desc == NULL)
    return FALSE;

  return TRUE;
}

/*
 * Give priority to the device which is opened before (open_cnt > 0)
 * and satisfies the requirement of major number and minor number.
 *
 * If such device does not exist, return a device which is not opened before
 * (open_cnt == 0) and satisfies the requirement of major number.
 */
int dev_open(major_no_t major_no, int minor_no, void *data) {
  const irq_state_t state = irq_protect();

  for (int i = 0; i < DEV_TABLE_SIZE; i++) {
    device_t *device = dev_table + i;
    if (device->desc->major_no == major_no &&
        device->minor_no == minor_no) {
      device->open_cnt++;
      irq_unprotect(state);
      return i;
    }
  }

  device_t *free_dev = NULL;
  for (int i = 0; i < DEV_TABLE_SIZE; i++) {
    device_t *device = dev_table + i;
    if (!device->open_cnt) {
      free_dev = device;
      break;
    }
  }

  dev_desc_t *dev_desc = NULL;
  for (size_t i = 0; i < sizeof(dev_desc_table) / sizeof(dev_desc_t *); i++) {
    dev_desc_t *curr_dev_desc = dev_desc_table[i];
    if (curr_dev_desc->major_no == major_no) {
      dev_desc = curr_dev_desc;
      break;
    }
  }

  if (dev_desc && free_dev) {
    free_dev->minor_no = minor_no;
    free_dev->data = data;
    free_dev->desc = dev_desc;

    if (!dev_desc->open(free_dev)) {
      free_dev->open_cnt = 1;
      irq_unprotect(state);
      return free_dev - dev_table;
    }
  }

  irq_unprotect(state);
  return -1;
}

int dev_close(int dev_id) {
  if (!is_dev_id_valid(dev_id))
    return -1;

  device_t *device = dev_table + dev_id;
  const irq_state_t state = irq_protect();

  if (--device->open_cnt == 0) {
    device->desc->close(device);
    kernel_memset(device, 0, sizeof(device_t));
  }

  irq_unprotect(state);
  return 0;
}

int dev_read(int dev_id, uint32_t addr, void *buf, size_t size) {
  if (!is_dev_id_valid(dev_id))
    return -1;

  const device_t *device = dev_table + dev_id;
  return device->desc->read(device, addr, buf, size);
}

int dev_write(int dev_id, uint32_t addr, const void *buf, size_t size) {
  if (!is_dev_id_valid(dev_id))
    return -1;

  const device_t *device = dev_table + dev_id;
  return device->desc->write(device, addr, buf, size);
}

int dev_control(int dev_id, int cmd, ...) {
  if (!is_dev_id_valid(dev_id))
    return -1;

  va_list arg_list;
  va_start(arg_list, cmd);
  const device_t *device = dev_table + dev_id;
  int return_value = device->desc->control(device, cmd, arg_list);
  va_end(arg_list);

  return return_value;
}
