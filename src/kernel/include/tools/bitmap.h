// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef BITMAP_H
#define BITMAP_H

#include "comm/types.h"

typedef struct _bitmap_t {
  size_t bits;
  uint8_t *bit_arr; // a two-dimensional array
} bitmap_t;

#define bmp_bytes_cnt(bits) (((bits) + 7) / 8)
#define bmp_get_bit(bmp, index)                                                \
  ({                                                                           \
    typeof(index) _index = (index);                                            \
    (bmp)->bit_arr[_index / 8] & (1 << (_index % 8));                          \
  })

#define bmp_is_set(bmp, index) ((bmp_get_bit(bmp, index)) ? 1 : 0)

void bmp_init(bitmap_t *bmp, uint8_t *bit_arr, int bits, int init_bit);
void bmp_set_bit(const bitmap_t *bmp, size_t index, size_t bit_cnt, _Bool bit);
int bmp_alloc_multi_bit(const bitmap_t *bmp, _Bool bit, size_t bit_cnt);

#endif
