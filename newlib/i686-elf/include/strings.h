// Copyright (c) 2002 Mike Barcroft <mike@FreeBSD.org>
// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _STRINGS_H_
#define	_STRINGS_H_

#include <sys/cdefs.h>
#include <sys/_types.h>

#if __POSIX_VISIBLE >= 200809
#include <sys/_locale.h>
#endif

#ifndef _SIZE_T_DECLARED
typedef	__size_t	size_t;
#define	_SIZE_T_DECLARED
#endif

__BEGIN_DECLS
#if __BSD_VISIBLE || __POSIX_VISIBLE <= 200112
int	 bcmp(const void *, const void *, size_t) __pure;	/* LEGACY */
void	 bcopy(const void *, void *, size_t);			/* LEGACY */
void	 bzero(void *, size_t);					/* LEGACY */
#endif
#if __BSD_VISIBLE
void	 explicit_bzero(void *, size_t);
#endif
#if __MISC_VISIBLE || __POSIX_VISIBLE < 200809 || __XSI_VISIBLE >= 700
int	 ffs(int) __pure2;
#endif
#if __BSD_VISIBLE
int	 ffsl(long) __pure2;
int	 ffsll(long long) __pure2;
int	 fls(int) __pure2;
int	 flsl(long) __pure2;
int	 flsll(long long) __pure2;
#endif
#if __BSD_VISIBLE || __POSIX_VISIBLE <= 200112
char	*index(const char *, int) __pure;			/* LEGACY */
char	*rindex(const char *, int) __pure;			/* LEGACY */
#endif
int	 strcasecmp(const char *, const char *) __pure;
int	 strncasecmp(const char *, const char *, size_t) __pure;

#if __POSIX_VISIBLE >= 200809
int	 strcasecmp_l (const char *, const char *, locale_t);
int	 strncasecmp_l (const char *, const char *, size_t, locale_t);
#endif
__END_DECLS

#if __SSP_FORTIFY_LEVEL > 0
#include <ssp/strings.h>
#endif

#endif /* _STRINGS_H_ */
