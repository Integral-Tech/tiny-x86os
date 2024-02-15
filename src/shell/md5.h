// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MD5_H
#define MD5_H

#include <stdint.h>
#include <stdio.h>

#define MD5SUM_LEN 16
#define MD5STR_LEN 32

#define INPUT_BUF_SIZE 1024

#define A 0x67452301
#define B 0xefcdab89
#define C 0x98badcfe
#define D 0x10325476

// Bit-manipulation functions defined by the MD5 algorithm
#define F(X, Y, Z)                                                             \
  ({                                                                           \
    typeof(X) _X = (X);                                                        \
    typeof(Y) _Y = (Y);                                                        \
    typeof(Z) _Z = (Z);                                                        \
    (_X & _Y) | (~_X & _Z);                                                    \
  })

#define G(X, Y, Z)                                                             \
  ({                                                                           \
    typeof(X) _X = (X);                                                        \
    typeof(Y) _Y = (Y);                                                        \
    typeof(Z) _Z = (Z);                                                        \
    (_X & _Z) | (_Y & ~_Z);                                                    \
  })

#define H(X, Y, Z)                                                             \
  ({                                                                           \
    typeof(X) _X = (X);                                                        \
    typeof(Y) _Y = (Y);                                                        \
    typeof(Z) _Z = (Z);                                                        \
    _X ^ _Y ^ _Z;                                                              \
  })

#define I(X, Y, Z)                                                             \
  ({                                                                           \
    typeof(X) _X = (X);                                                        \
    typeof(Y) _Y = (Y);                                                        \
    typeof(Z) _Z = (Z);                                                        \
    _Y ^ (_X | ~_Z);                                                           \
  })

// Rotates a 32-bit word left by n bits
#define rotate_left(x, n)                                                      \
  ({                                                                           \
    typeof(x) _x = x;                                                          \
    typeof(n) _n = n;                                                          \
    (_x << _n) | (_x >> (32 - _n));                                            \
  })

typedef struct _md5_context_t {
  uint64_t size;              // Size of input in bytes
  uint32_t buffer[4];         // Current accumulation of hash
  uint8_t input[64];          // Input to be used in the next step
  uint8_t digest[MD5SUM_LEN]; // Result of algorithm
} md5_context_t;

void md5_init(md5_context_t *ctx);
void md5_update(md5_context_t *ctx, uint8_t *input, size_t input_len);
void md5_finalize(md5_context_t *ctx);
void md5_step(uint32_t *buffer, uint32_t *input);

void md5_string(char *input, uint8_t *result);
void md5_file(FILE *file, uint8_t *result);

#endif
