// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TYPES_H
#define TYPES_H

#if !defined(__ASSEMBLER__)

#ifndef _UINT8_T_DECLARED
#define _UINT8_T_DECLARED
typedef unsigned char uint8_t;
#endif

#ifndef _UINT16_T_DECLARED
#define _UINT16_T_DECLARED
typedef unsigned short uint16_t;
#endif

#ifndef _UINT32_T_DECLARED
#define _UINT32_T_DECLARED
typedef unsigned long uint32_t;
#endif

#ifndef _UINT64_T_DECLARED
#define _UINT64_T_DECLARED
typedef unsigned long long uint64_t;
#endif

#ifndef _SIZE_T_DECLARED
#define _SIZE_T_DECLARED
typedef unsigned int size_t;
#endif

#ifndef _PTRDIFF_T_DECLARED
#define _PTRDIFF_T_DECLARED
typedef int ptrdiff_t;
#endif

#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

#define TRUE 1
#define FALSE 0

#endif
