// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TIME_H
#define TIME_H

#define PIT_OSC_FREQ 1193182 // Programmable Interval Timer
#define PIT_COMMAND_MODE_PORT 0x43
#define PIT_CHANNEL0_DATA_PORT 0x40

#define PIT_CHANNEL0 (0 << 6) // select counter 0
#define PIT_LOAD_LOHI                                                          \
  (3 << 4) // load the lower byte first, then the higher byte
#define PIT_MODE3 (3 << 1)

void time_init();
void exception_handler_time();

#endif