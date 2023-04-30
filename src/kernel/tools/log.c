#include "tools/log.h"
#include "comm/cpu_instr.h"
#include "dev/dev.h"
#include "ipc/mutex.h"
#include "tools/klib.h"

static mutex_t mutex;
static int log_dev_id;

void log_init() {
  mutex_init(&mutex);
  log_dev_id = dev_open(TTY_DEV, 0, NULL);

#if LOG_COM
  outb(COM1_PORT + 1, 0x00);
  outb(COM1_PORT + 3, 0x80);
  outb(COM1_PORT, 0x03);
  outb(COM1_PORT + 1, 0x00);
  outb(COM1_PORT + 3, 0x03);
  outb(COM1_PORT + 2, 0xC7);
  outb(COM1_PORT + 4, 0x0F);
#endif
}

int log_printf(const char *fmt, ...) {
  char str_buf[BUF_SIZE];
  kernel_memset(str_buf, 0, sizeof(str_buf));

  va_list args;
  va_start(args, fmt);
  const int length = kernel_vsprintf(str_buf, fmt, args);
  va_end(args);

  mutex_lock(&mutex);

#if LOG_COM
  const char *p = str_buf;
  while (*p != '\0') {
    while (!inb((COM1_PORT + 5) & (1 << 6)))
      ; // Check whether the port is busy
    outb(COM1_PORT, *p++);
  }

  outb(COM1_PORT, '\r'); // carriage return
  outb(COM1_PORT, '\n'); // new line
#else
  const char newline = '\n';
  dev_write(log_dev_id, 0, str_buf, kernel_strlen(str_buf));
  dev_write(log_dev_id, 0, &newline, 1);
#endif

  mutex_unlock(&mutex);
  return length;
}
