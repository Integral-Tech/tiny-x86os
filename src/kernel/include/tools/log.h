// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOG_H
#define LOG_H

#define COM1_PORT 0x3F8
#define BUF_SIZE 128

#define LOG_COM 0

void log_init();
int log_printf(const char *fmt, ...);

#endif
