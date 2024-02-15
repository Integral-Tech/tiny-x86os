/*	$NetBSD: grp.h,v 1.7 1995/04/29 05:30:40 cgd Exp $	*/

// Copyright (c) 1989, 1993
// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _GRP_H_
#define	_GRP_H_

#include <sys/cdefs.h>
#include <sys/types.h>
#ifdef __CYGWIN__
#include <cygwin/grp.h>
#endif

#if __BSD_VISIBLE
#define	_PATH_GROUP		"/etc/group"
#endif

struct group {
	char	*gr_name;		/* group name */
	char	*gr_passwd;		/* group password */
	gid_t	gr_gid;			/* group id */
	char	**gr_mem;		/* group members */
};

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __INSIDE_CYGWIN__
struct group	*getgrgid (gid_t);
struct group	*getgrnam (const char *);
#if __MISC_VISIBLE || __POSIX_VISIBLE
int		 getgrnam_r (const char *, struct group *,
			char *, size_t, struct group **);
int		 getgrgid_r (gid_t, struct group *,
			char *, size_t, struct group **);
#endif /* __MISC_VISIBLE || __POSIX_VISIBLE */
#if __MISC_VISIBLE || __XSI_VISIBLE >= 4
struct group	*getgrent (void);
void		 setgrent (void);
void		 endgrent (void);
#endif /* __MISC_VISIBLE || __XSI_VISIBLE >= 4 */
#if __BSD_VISIBLE
int		 initgroups (const char *, gid_t);
#endif /* __BSD_VISIBLE */
#endif /* !__INSIDE_CYGWIN__ */

#ifdef __cplusplus
}
#endif

#endif /* !_GRP_H_ */
