#include "tools/klib.h"
#include "comm/cpu_instr.h"
#include "tools/log.h"

char *kernel_strcpy(char *dest, const char *src) {
  if (!dest || !src)
    return dest; // judge whether dest or src is NULL

  const char *p = dest;
  while (*src != '\0')
    *dest++ = *src++;

  *dest = '\0';
  return (char *)p;
}

char *kernel_strncpy(char *dest, const char *src, size_t size) {
  if (!dest || !src || !size)
    return dest; // judge whether dest/src is NULL, or the size equals 0

  const char *p = dest;
  while (*src != '\0' && size--)
    *dest++ = *src++;

  if (size > 0)
    *dest = '\0';

  return (char *)p;
}

int kernel_strcmp(const char *s1, const char *s2) {
  if (!s1 || !s2)
    return -1;

  while (*s1 != '\0' && *s2 != '\0' && *s1 == *s2) {
    s1++;
    s2++;
  }

  return *s1 - *s2;
}

int kernel_strncmp(const char *s1, const char *s2, size_t size) {
  if (!s1 || !s2 || !size)
    return -1;

  while (*s1 != '\0' && *s2 != '\0' && *s1 == *s2 && size--) {
    s1++;
    s2++;
  }

  if (!size && *(s1 - 1) == *(s2 - 1))
    return 0;
  else
    return *s1 - *s2;
}

size_t kernel_strlen(const char *str) {
  if (!str)
    return 0;

  size_t length = 0;
  while (*str++ != '\0')
    length++;

  return length;
}

char *kernel_strlwr(char *str) {
  if (!str)
    return NULL;

  for (char *ptr = str; *ptr != '\0'; ptr++)
    if (*ptr >= 'A' && *ptr <= 'Z')
      tolower(*ptr);

  return str;
}

void *kernel_memcpy(void *dest, const void *src, size_t size) {
  if (!dest || !src || !size)
    return dest;

  const uint8_t *pSrc = (uint8_t *)src;
  uint8_t *pDest = (uint8_t *)dest;

  while (size--)
    *pDest++ = *pSrc++;

  return dest;
}

void *kernel_memset(void *dest, uint8_t data, size_t size) {
  if (!dest || !size)
    return dest;

  uint8_t *pDest = (uint8_t *)dest;
  while (size--)
    *pDest++ = data;

  return dest;
}

int kernel_memcmp(const void *data1, const void *data2, size_t size) {
  if (!data1 || !data2 || !size)
    return -1;

  const uint8_t *pData1 = (uint8_t *)data1, *pData2 = (uint8_t *)data2;
  while (size-- && *pData1++ == *pData2++)
    ;

  return *pData1 - *pData2;
}

void num2str(int num, char *buf, int radix) {
  static const char *num2char = "FEDCBA9876543210123456789ABCDEF";
  if (radix != 8 && radix != 10 && radix != 16)
    return;

  char *curr = buf;
  if ((num < 0 && radix == 10) || num >= 0) {
    char *start = buf;
    if (num < 0) {
      *curr++ = '-';
      start++;
    }
    do {
      const char c = num2char[num % radix + 15];
      *curr++ = c;
      num /= radix;
    } while (num);

    *curr-- = '\0';
    while (start < curr) {
      const char tmp = *start;
      *start++ = *curr;
      *curr-- = tmp;
    }
  } else {
    /*
     * int -> arithmatic shift
     * unsigned int -> logical shift
     * uNum = num convert arithmatic shift to logical shift.
     */
    unsigned int mask;
    const unsigned int uNum = num;

    if (radix == 8) {
      mask = 03 << 30;
      *curr++ = num2char[((uNum & mask) >> 30) + 15];

      mask = 07 << 27;
      for (int i = 27; i >= 0; i -= 3) {
        *curr++ = num2char[((uNum & mask) >> i) + 15];
        mask >>= 3;
      }
    } else {
      mask = 0xF << 28;
      for (int i = 28; i >= 0; i -= 4) {
        *curr++ = num2char[((uNum & mask) >> i) + 15];
        mask >>= 4;
      }
    }
    *curr-- = '\0';
  }
}

int str2num_dec(const char *str, int *num) {
  if (str == NULL || *str == '\0')
    return -1;

  int target_num = 0;
  do {
    switch (*str) {
    case '0' ... '9':
      target_num = target_num * 10 + *str - '0';
      break;
    case '\0':
      break;
    default:
      return -1;
    }
  } while (*str++ != '\0');

  *num = target_num;
  return 0;
}

int kernel_vsprintf(char *str, const char *fmt, va_list arg) {
  const char *pFmt = fmt;
  char *pStr = str;
  while (*pFmt != '\0') {
    if (*pFmt == '%') {
      switch (*(pFmt + 1)) {
      case 's':
        kernel_strcpy(pStr, va_arg(arg, char *));
        break;
      case 'd':
        num2str(va_arg(arg, int), pStr, 10);
        break;
      case 'o':
      case 'O':
        num2str(va_arg(arg, int), pStr, 8);
        break;
      case 'x':
      case 'X':
        num2str(va_arg(arg, int), pStr, 16);
        break;
      case 'c':
        *pStr++ = va_arg(arg, int);
        pFmt += 2;
        continue;
      }
      pStr += kernel_strlen(pStr);
      pFmt += 2;
    } else
      *pStr++ = *pFmt++;
  }
  return pStr - str - 1;
}

int kernel_sprintf(char *str, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  const int length = kernel_vsprintf(str, fmt, args);
  va_end(args);
  return length;
}

char *kernel_strrchr(const char *str, int c) {
  if (!str)
    return (char *)str;

  char *ptr = (char *)(str + kernel_strlen(str));

  do {
    ptr--;
  } while (ptr >= str && *ptr != c);

  return ptr;
}

int strings_cnt(char *const *start) {
  int count = 0;

  if (start)
    while (*start++)
      count++;

  return count;
}

const char *kernel_basename(const char *path) {
  const char *tail = kernel_strrchr(path, '/');
  return tail ? tail + 1 : path;
}

_Bool str_begin_with(const char *s1, const char *s2) {
  if (!s1 || !s2)
    return FALSE;

  while (*s1 != '\0' && *s2 != '\0' && *s1 == *s2) {
    s1++;
    s2++;
  }

  return *s2 == '\0';
}

void panic(const char *file, const int line, const char *func,
           const char *detail) {
  log_printf("Assert failed: %s", detail);
  log_printf("File: %s, Line: %d, Function: %s", file, line, func);
  while (1) {
    hlt();
  }
}
