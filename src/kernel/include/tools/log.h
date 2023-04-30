#ifndef LOG_H
#define LOG_H

#define COM1_PORT 0x3F8
#define BUF_SIZE 128

#define LOG_COM 0

void log_init();
int log_printf(const char *fmt, ...);

#endif
