// Copyright (C) 1991 DJ Delorie
// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

/* Modified to use SETJMP_DJ_H rather than SETJMP_H to avoid
   conflicting with setjmp.h.  Ian Taylor, Cygnus support, April,
   1993.  */

#ifndef _SETJMP_DJ_H_
#define _SETJMP_DJ_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  unsigned long eax;
  unsigned long ebx;
  unsigned long ecx;
  unsigned long edx;
  unsigned long esi;
  unsigned long edi;
  unsigned long ebp;
  unsigned long esp;
  unsigned long eip;
} jmp_buf[1];

extern int setjmp(jmp_buf);
extern void longjmp(jmp_buf, int);

#ifdef __cplusplus
}
#endif

#endif
