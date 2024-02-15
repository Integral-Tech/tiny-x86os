// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dev/tty.h"
#include "cpu/irq.h"
#include "dev/console.h"
#include "dev/keyboard.h"
#include "tools/klib.h"
#include "tools/log.h"

const dev_desc_t tty_desc = {.name = "tty",
                             .major_no = TTY_DEV,
                             .open = tty_open,
                             .close = tty_close,
                             .read = tty_read,
                             .write = tty_write,
                             .control = tty_control};

static tty_t tty_dev_table[TTY_NUM];
static int curr_tty_id = 0;

static int get_tty_id(const device_t *dev) {
  const int tty_id = dev->minor_no;
  if (tty_id < 0 || tty_id >= TTY_NUM) {
    log_printf("tty%d is invalid!", tty_id);
    return -1;
  }

  return tty_id;
}

void tty_queue_init(tty_queue_t *queue, char *buf, int size) {
  queue->buf = buf;
  queue->count = 0;
  queue->size = size;
  queue->read = queue->write = 0;
}

int tty_enqueue(tty_queue_t *queue, char data) {
  irq_state_t state = irq_protect();

  if (queue->count >= queue->size) {
    irq_unprotect(state);
    return -1;
  } // FIFO is full

  queue->buf[queue->write++] = data;
  if (queue->write >= queue->size)
    queue->write = 0;

  queue->count++;
  irq_unprotect(state);
  return 0;
}

int tty_dequeue(tty_queue_t *queue, char *data) {
  irq_state_t state = irq_protect();

  if (queue->count <= 0) {
    irq_unprotect(state);
    return -1;
  }

  *data = queue->buf[queue->read++];
  if (queue->read >= queue->size)
    queue->read = 0;

  queue->count--;
  irq_unprotect(state);
  return 0;
}

int tty_open(const device_t *dev) {
  const int tty_id = get_tty_id(dev);
  if (tty_id < 0)
    return -1;

  tty_t *tty = tty_dev_table + tty_id;
  tty_queue_init(&tty->output_queue, tty->output_buf, TTY_OUTPUT_BUF_SIZE);
  tty_queue_init(&tty->input_queue, tty->input_buf, TTY_INPUT_BUF_SIZE);
  tty->console_id = tty_id;
  tty->output_crlf_esc = tty->input_crlf_esc = TRUE; // enable CRLF escape
  tty->input_echo = TRUE;
  sem_init(&tty->output_sem, TTY_OUTPUT_BUF_SIZE);
  sem_init(&tty->input_sem, 0);

  keyboard_init();
  console_init(tty_id);
  return 0;
}

int tty_close(const device_t *dev) { return 0; }

int tty_read(const device_t *dev, uint32_t addr, void *buf, size_t size) {
  const int tty_id = get_tty_id(dev);
  if (tty_id < 0 || !dev->open_cnt || !size)
    return -1;

  tty_t *tty = tty_dev_table + tty_id;
  size_t len = 0;
  char *ptr_buf = buf;
  while (len < size) {
    sem_wait(&tty->input_sem);
    char ch;
    tty_dequeue(&tty->input_queue, &ch);
    switch (ch) {
    case '\b':
    case ASCII_DEL:
      if (!len)
        continue;

      len--;
      ptr_buf--;
      break;
    case '\n':
      if (tty->input_crlf_esc && len + 1 < size) {
        *ptr_buf++ = '\r';
        len++;
      }

      *ptr_buf++ = '\n';
      len++;
      break;
    case '\t':
      if (len + TAB_SIZE - 1 < size) {
        kernel_strcpy(ptr_buf, "    ");
        ptr_buf += TAB_SIZE;
        len += TAB_SIZE;
      }
      break;
    default:
      *ptr_buf++ = ch;
      len++;
    }

    if (tty->input_echo)
      tty_write(dev, 0, &ch, 1);

    if (ch == '\n')
      break;
  }

  return len;
}

int tty_write(const device_t *dev, uint32_t addr, const void *buf,
              size_t size) {
  const int tty_id = get_tty_id(dev);
  if (tty_id < 0 || !dev->open_cnt || !size)
    return -1;

  const size_t initial_size = size;
  tty_t *tty = tty_dev_table + tty_id;
  const char *ptr_buf = buf;
  while (size--) {
    const char ch = *ptr_buf++;
    if (ch == '\n' && tty->output_crlf_esc) {
      sem_wait(&tty->output_sem);
      if (tty_enqueue(&tty->output_queue, '\r') < 0)
        break;
    }

    sem_wait(&tty->output_sem);
    if (tty_enqueue(&tty->output_queue, ch) < 0)
      break;

    console_write(tty);
  } // convert '\n' to '\r\n'

  return initial_size - size + 1;
}

int tty_control(const device_t *dev, int cmd, va_list arg_list) {
  const int tty_id = get_tty_id(dev);
  if (tty_id < 0)
    return -1;

  tty_t *tty = tty_dev_table + tty_id;

  switch (cmd) {
  case TTY_CMD_ECHO:
    if (va_arg(arg_list, void *))
      tty->input_echo = TRUE;
    else
      tty->input_echo = FALSE;
    break;
  default:
    break;
  }

  return 0;
}

int tty_in(char data) {
  tty_t *tty = tty_dev_table + curr_tty_id;
  if (sem_cnt(&tty->input_sem) >= TTY_INPUT_BUF_SIZE)
    return -1; // buffer is full

  tty_enqueue(&tty->input_queue, data);
  sem_notify(&tty->input_sem);
  return 0;
}

void tty_switch_to(int tty_id) {
  if (tty_id != curr_tty_id) {
    console_switch_to(tty_id);
    curr_tty_id = tty_id;
  }
}
