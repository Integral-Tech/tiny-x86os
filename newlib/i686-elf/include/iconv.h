// Copyright (c) 2003-2004, Artem B. Bityuckiy, SoftMine Corporation.
// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _ICONV_H_
#define _ICONV_H_

#include <_ansi.h>
#include <reent.h>
#include <sys/types.h>
#include <sys/_types.h>

/* iconv_t: charset conversion descriptor type */
typedef _iconv_t iconv_t;

_BEGIN_STD_C

#ifndef _REENT_ONLY
iconv_t
iconv_open (const char *, const char *);

size_t
iconv (iconv_t, char **__restrict, size_t *__restrict, 
               char **__restrict, size_t *__restrict);

int
iconv_close (iconv_t);
#endif

iconv_t
_iconv_open_r (struct _reent *, const char *, const char *);

size_t
_iconv_r (struct _reent *, iconv_t, const char **,
                  size_t *, char **, size_t *);

int
_iconv_close_r (struct _reent *, iconv_t);

_END_STD_C

#endif /* #ifndef _ICONV_H_ */
