/* From curses.h.  */

// Copyright (c) 1981, 1993
// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _UNCTRL_H_
#define _UNCTRL_H_

#include <_ansi.h>

#define unctrl(c)		__unctrl[(c) & 0xff]
#define unctrllen(ch)		__unctrllen[(ch) & 0xff]

extern __IMPORT const char * const __unctrl[256];	/* Control strings. */
extern __IMPORT const char __unctrllen[256];	/* Control strings length. */

#endif /* _UNCTRL_H_ */
