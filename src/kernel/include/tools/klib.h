// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef KLIB_H
#define KLIB_H

#include "comm/types.h"
#include <stdarg.h>

#define down2(size, bound)                                                     \
  ({                                                                           \
    typeof(size) _size = (size);                                               \
    typeof(bound) _bound = (bound);                                            \
    _size & ~(_bound - 1);                                                     \
  })

#define up2(size, bound)                                                       \
  ({                                                                           \
    typeof(size) _size = (size);                                               \
    typeof(bound) _bound = (bound);                                            \
    (_size + _bound - 1) & ~(_bound - 1);                                      \
  })

char *kernel_strcpy(char *dest, const char *src);
char *kernel_strncpy(char *dest, const char *src, size_t size);
int kernel_strncmp(const char *s1, const char *s2, size_t size);
int kernel_strcmp(const char *s1, const char *s2);
size_t kernel_strlen(const char *str);
char *kernel_strrchr(const char *str, int c);
_Bool str_begin_with(const char *s1, const char *s2);
char *kernel_strlwr(char *str);

void *kernel_memcpy(void *dest, const void *src, size_t size);
void *kernel_memset(void *dest, uint8_t data, size_t size);
int kernel_memcmp(const void *data1, const void *data2, size_t size);

void num2str(int num, char *buf, int radix);
int str2num_dec(const char *str, int *num);

int kernel_vsprintf(char *str, const char *fmt, va_list arg);
int kernel_sprintf(char *str, const char *fmt, ...);

void panic(const char *file, const int line, const char *func,
           const char *detail);

#ifndef RELEASE
#define ASSERT(expr)                                                           \
  if (!(expr))                                                                 \
  panic(__FILE__, __LINE__, __func__, #expr)
#else
#define ASSERT(expr)
#endif

int strings_cnt(char *const *start);
const char *kernel_basename(const char *path);

#define max(a, b)                                                              \
  ({                                                                           \
    typeof(a) _a = (a);                                                        \
    typeof(b) _b = (b);                                                        \
    (void)(&_a == &_b);                                                        \
    _a > _b ? _a : _b;                                                         \
  })

#define min(a, b)                                                              \
  ({                                                                           \
    typeof(a) _a = (a);                                                        \
    typeof(b) _b = (b);                                                        \
    (void)(&_a == &_b);                                                        \
    _a < _b ? _a : _b;                                                         \
  })

#define toupper(ch) (ch) += 'A' - 'a';
#define tolower(ch) (ch) += 'a' - 'A';

#define streq(s1, s2) (!kernel_strcmp(s1, s2))

#define ARR_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#endif
