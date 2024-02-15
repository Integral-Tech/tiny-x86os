/*	$NetBSD: ar.h,v 1.4 1994/10/26 00:55:43 cgd Exp $	*/

// Copyright (c) 1991, 1993
// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _AR_H_
#define	_AR_H_

/* Pre-4BSD archives had these magic numbers in them. */
#define	OARMAG1	0177555
#define	OARMAG2	0177545

#define	ARMAG		"!<arch>\n"	/* ar "magic number" */
#define	SARMAG		8		/* strlen(ARMAG); */

#define	AR_EFMT1	"#1/"		/* extended format #1 */

struct ar_hdr {
	char ar_name[16];		/* name */
	char ar_date[12];		/* modification time */
	char ar_uid[6];			/* user id */
	char ar_gid[6];			/* group id */
	char ar_mode[8];		/* octal file permissions */
	char ar_size[10];		/* size in bytes */
#define	ARFMAG	"`\n"
	char ar_fmag[2];		/* consistency check */
};

#endif /* !_AR_H_ */
