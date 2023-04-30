#ifndef TTY_H
#define TTY_H

#include "dev/dev.h"
#include "ipc/sem.h"

#define TTY_NUM 8

#define TTY_OUTPUT_BUF_SIZE 512
#define TTY_INPUT_BUF_SIZE 512

#define TAB_SIZE 4

#define TTY_CMD_ECHO 1

typedef struct _tty_queue_t {
  char *buf;
  int size;
  int read, write;
  int count;
} tty_queue_t;

typedef struct _tty_t {
  tty_queue_t output_queue, input_queue;
  char output_buf[TTY_OUTPUT_BUF_SIZE], input_buf[TTY_INPUT_BUF_SIZE];
  sem_t output_sem, input_sem;
  _Bool output_crlf_esc, input_crlf_esc; // convert '\n' to "\r\n"
  _Bool input_echo;
  int console_id;
} tty_t;

int tty_open(const device_t *dev);
int tty_close(const device_t *dev);
int tty_read(const device_t *dev, uint32_t addr, void *buf, size_t size);
int tty_write(const device_t *dev, uint32_t addr, const void *buf, size_t size);
int tty_control(const device_t *dev, int cmd, va_list arg_list);
void tty_queue_init(tty_queue_t *queue, char *buf, int size);
int tty_enqueue(tty_queue_t *queue, char data);
int tty_dequeue(tty_queue_t *queue, char *data);
int tty_in(char data);
void tty_switch_to(int tty_id);

#endif
