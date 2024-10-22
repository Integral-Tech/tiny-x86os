// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "comm/types.h"

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STAT_PORT 0x64
#define KEYBOARD_CMD_PORT 0x64
#define KEYBOARD_STAT_READY (1 << 0)

#define KEY_LSHIFT 0x2A
#define KEY_RSHIFT 0x36
#define KEY_CAPS_LOCK 0x3A

#define KEY_CTRL 0x1D
#define KEY_LSHIFT 0x2A
#define KEY_RSHIFT 0x36
#define KEY_ALT 0x38

#define KEY_F1 0x3B
#define KEY_F2 0x3C
#define KEY_F3 0x3D
#define KEY_F4 0x3E
#define KEY_F5 0x3F
#define KEY_F6 0x40
#define KEY_F7 0x41
#define KEY_F8 0x42
#define KEY_F9 0x43
#define KEY_F10 0x44
#define KEY_F11 0x57
#define KEY_F12 0x58

#define is_make_code(key_code) !((key_code) & 0x80)

typedef struct _key_map_t {
  uint8_t normal, func;
} key_map_t;

typedef struct _keyboard_stat_t {
  _Bool caps_lock;
  _Bool ctrl_key, shift_key, alt_key;
} keyboard_stat_t;

void keyboard_init();
void exception_handler_keyboard();

#endif
