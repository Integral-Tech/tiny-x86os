// Copyright (c) 2003-2004, Artem B. Bityuckiy.
// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * Funtions, macros, etc implimented in iconv library but used by other
 * NLS-related subsystems too.
 */
#ifndef __SYS_ICONVNLS_H__
#define __SYS_ICONVNLS_H__

#include <_ansi.h>
#include <reent.h>
#include <wchar.h>
#include <iconv.h>

/* Iconv data path environment variable name */
#define NLS_ENVVAR_NAME  "NLSPATH"
/* Default NLSPATH value */
#define ICONV_DEFAULT_NLSPATH "/usr/locale"
/* Direction markers */
#define ICONV_NLS_FROM 0
#define ICONV_NLS_TO   1

void
_iconv_nls_get_state (iconv_t cd, mbstate_t *ps, int direction);

int
_iconv_nls_set_state (iconv_t cd, mbstate_t *ps, int direction);

int
_iconv_nls_is_stateful (iconv_t cd, int direction);

int
_iconv_nls_get_mb_cur_max (iconv_t cd, int direction);

size_t
_iconv_nls_conv (struct _reent *rptr, iconv_t cd,
                        const char **inbuf, size_t *inbytesleft,
                        char **outbuf, size_t *outbytesleft);

const char *
_iconv_nls_construct_filename (struct _reent *rptr, const char *file,
                                      const char *dir, const char *ext);


int
_iconv_nls_open (struct _reent *rptr, const char *encoding,
                        iconv_t *towc, iconv_t *fromwc, int flag);

char *
_iconv_resolve_encoding_name (struct _reent *rptr, const char *ca);

#endif /* __SYS_ICONVNLS_H__ */

