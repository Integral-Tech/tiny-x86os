/*	$NetBSD: strings.h,v 1.3 2008/04/28 20:22:54 martin Exp $	*/

// Copyright (c) 2007 The NetBSD Foundation, Inc.
// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _SSP_STRINGS_H_
#define _SSP_STRINGS_H_

#include <ssp/ssp.h>

#if __SSP_FORTIFY_LEVEL > 0

#if __BSD_VISIBLE || __POSIX_VISIBLE <= 200112
#define bcopy(src, dst, len) \
    ((__ssp_bos0(dst) != (size_t)-1) ? \
    __builtin___memmove_chk(dst, src, len, __ssp_bos0(dst)) : \
    __memmove_ichk(dst, src, len))
#define bzero(dst, len) \
    ((__ssp_bos0(dst) != (size_t)-1) ? \
    __builtin___memset_chk(dst, 0, len, __ssp_bos0(dst)) : \
    __memset_ichk(dst, 0, len))
#endif

#if __BSD_VISIBLE
__ssp_redirect0(void, explicit_bzero, (void *__buf, size_t __len), \
    (__buf, __len));
#endif

#endif /* __SSP_FORTIFY_LEVEL > 0 */
#endif /* _SSP_STRINGS_H_ */
