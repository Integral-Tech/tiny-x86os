/*	$NetBSD: stdio.h,v 1.5 2011/07/17 20:54:34 joerg Exp $	*/

// Copyright (c) 2006 The NetBSD Foundation, Inc.
// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _SSP_STDIO_H_
#define _SSP_STDIO_H_

#include <ssp/ssp.h>

__BEGIN_DECLS
int __sprintf_chk(char *__restrict, int, size_t, const char *__restrict, ...)
    __printflike(4, 5);
int __vsprintf_chk(char *__restrict, int, size_t, const char *__restrict,
    __va_list)
    __printflike(4, 0);
int __snprintf_chk(char *__restrict, size_t, int, size_t,
    const char *__restrict, ...)
    __printflike(5, 6);
int __vsnprintf_chk(char *__restrict, size_t, int, size_t,
     const char *__restrict, __va_list)
    __printflike(5, 0);
char *__gets_chk(char *, size_t);
__END_DECLS

#if __SSP_FORTIFY_LEVEL > 0


#define sprintf(str, ...) \
    __builtin___sprintf_chk(str, 0, __ssp_bos(str), __VA_ARGS__)

#define vsprintf(str, fmt, ap) \
    __builtin___vsprintf_chk(str, 0, __ssp_bos(str), fmt, ap)

#define snprintf(str, len, ...) \
    __builtin___snprintf_chk(str, len, 0, __ssp_bos(str), __VA_ARGS__)

#define vsnprintf(str, len, fmt, ap) \
    __builtin___vsnprintf_chk(str, len, 0, __ssp_bos(str), fmt, ap)

#define gets(str) \
    __gets_chk(str, __ssp_bos(str))

__ssp_decl(char *, fgets, (char *__restrict __buf, int __len, FILE *__fp))
{
  if (__len > 0)
    __ssp_check(__buf, (size_t)__len, __ssp_bos);
  return __ssp_real_fgets(__buf, __len, __fp);
}

#if __GNU_VISIBLE
__ssp_decl(char *, fgets_unlocked, (char *__restrict __buf, int __len, FILE *__fp))
{
  if (__len > 0)
    __ssp_check(__buf, (size_t)__len, __ssp_bos);
  return __ssp_real_fgets_unlocked(__buf, __len, __fp);
}
#endif /* __GNU_VISIBLE */

__ssp_decl(size_t, fread, (void *__restrict __ptr, size_t __size, size_t __n, FILE *__restrict __fp))
{
  __ssp_check(__ptr, __size * __n, __ssp_bos0);
  return __ssp_real_fread(__ptr, __size, __n, __fp);
}

#if __MISC_VISIBLE
__ssp_decl(size_t, fread_unlocked, (void *__restrict __ptr, size_t __size, size_t __n, FILE *__restrict __fp))
{
  __ssp_check(__ptr, __size * __n, __ssp_bos0);
  return __ssp_real_fread_unlocked(__ptr, __size, __n, __fp);
}
#endif /* __MISC_VISIBLE */

#endif /* __SSP_FORTIFY_LEVEL > 0 */

#endif /* _SSP_STDIO_H_ */
