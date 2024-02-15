// Copyright (c) 1989 The Regents of the University of California.
// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _PWD_H_
#ifdef __cplusplus
extern "C" {
#endif
#define	_PWD_H_

#include <sys/cdefs.h>
#include <sys/types.h>

#if __BSD_VISIBLE
#define	_PATH_PASSWD		"/etc/passwd"

#define	_PASSWORD_LEN		128	/* max length, not counting NULL */
#endif

struct passwd {
	char	*pw_name;		/* user name */
	char	*pw_passwd;		/* encrypted password */
	uid_t	pw_uid;			/* user uid */
	gid_t	pw_gid;			/* user gid */
	char	*pw_comment;		/* comment */
	char	*pw_gecos;		/* Honeywell login info */
	char	*pw_dir;		/* home directory */
	char	*pw_shell;		/* default shell */
};

#ifndef __INSIDE_CYGWIN__
struct passwd	*getpwuid (uid_t);
struct passwd	*getpwnam (const char *);

#if __MISC_VISIBLE || __POSIX_VISIBLE
int 		 getpwnam_r (const char *, struct passwd *,
			char *, size_t , struct passwd **);
int		 getpwuid_r (uid_t, struct passwd *, char *,
			size_t, struct passwd **);
#endif

#if __MISC_VISIBLE || __XSI_VISIBLE >= 4
struct passwd	*getpwent (void);
void		 setpwent (void);
void		 endpwent (void);
#endif

#if __BSD_VISIBLE
int		 setpassent (int);
#endif
#endif /*!__INSIDE_CYGWIN__*/

#ifdef __cplusplus
}
#endif
#endif /* _PWD_H_ */
