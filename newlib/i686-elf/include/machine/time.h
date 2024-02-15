// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef	_MACHTIME_H_
#define	_MACHTIME_H_

#if defined(__rtems__) || defined(__VISIUM__) || defined(__riscv)
#define _CLOCKS_PER_SEC_ 1000000
#elif defined(__aarch64__) || defined(__arm__) || defined(__thumb__)
#define _CLOCKS_PER_SEC_ 100
#endif

#ifdef __SPU__
#include <sys/_timespec.h>
int nanosleep (const struct timespec *, struct timespec *);
#endif

#endif	/* _MACHTIME_H_ */
