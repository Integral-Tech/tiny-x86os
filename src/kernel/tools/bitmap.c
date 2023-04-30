#include "tools/bitmap.h"
#include "tools/klib.h"

static size_t last_alloc_index = 0;

void bmp_init(bitmap_t *bmp, uint8_t *bit_arr, int bits, int init_bit) {
  bmp->bits = bits;
  bmp->bit_arr = bit_arr;

  const int bytes = bmp_bytes_cnt(bmp->bits);
  kernel_memset(bmp->bit_arr, init_bit ? 0xFF : 0, bytes);
}

void bmp_set_bit(const bitmap_t *bmp, size_t index, size_t bit_cnt, _Bool bit) {
  for (size_t i = 0; (i < bit_cnt) && (index < bmp->bits); i++, index++) {
    if (bit)
      bmp->bit_arr[index / 8] |= (1 << (index % 8));
    else
      bmp->bit_arr[index / 8] &= ~(1 << (index % 8));
  }
}

int bmp_alloc_multi_bit(const bitmap_t *bmp, _Bool bit, size_t bit_cnt) {
  size_t curr_index = last_alloc_index, i;
  int start_index = -1;

  while (curr_index < bmp->bits) {
    if (bmp_get_bit(bmp, curr_index) != bit) {
      curr_index++;
      continue;
    }

    start_index = curr_index;
    for (i = 0; i < bit_cnt && curr_index < bmp->bits; i++) {
      if (bmp_get_bit(bmp, curr_index++) != bit) {
        start_index = -1;
        break;
      }
    }

    if (i >= bit_cnt) {
      bmp_set_bit(bmp, start_index, bit_cnt, !bit);
      last_alloc_index = curr_index;
      return start_index;
    }
  }

  return -1;
}
