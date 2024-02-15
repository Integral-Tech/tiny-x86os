// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <stdint.h>
#include <stdlib.h>

extern int main(int argc, char **argv);
extern uint8_t BSS_START[], BSS_END[];

void cStart(int argc, char **argv) {
  uint8_t *ptr = BSS_START;
  while (ptr <= BSS_END)
    *ptr++ = 0;

  exit(main(argc, argv));
}
