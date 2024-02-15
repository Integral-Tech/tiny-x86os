// Copyright (c) 2002 Mike Barcroft <mike@FreeBSD.org>
// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _SYS__TIMEVAL_H_
#define _SYS__TIMEVAL_H_

#include <sys/_types.h>

#ifndef _SUSECONDS_T_DECLARED
typedef	__suseconds_t	suseconds_t;
#define	_SUSECONDS_T_DECLARED
#endif

#if !defined(__time_t_defined) && !defined(_TIME_T_DECLARED)
typedef	_TIME_T_	time_t;
#define	__time_t_defined
#define	_TIME_T_DECLARED
#endif

/* This define is also used outside of Newlib, e.g. in MinGW-w64 */
#ifndef _TIMEVAL_DEFINED
#define _TIMEVAL_DEFINED

/*
 * Structure returned by gettimeofday(2) system call, and used in other calls.
 */
struct timeval {
	time_t		tv_sec;		/* seconds */
	suseconds_t	tv_usec;	/* and microseconds */
};
#endif /* _TIMEVAL_DEFINED */

#endif /* !_SYS__TIMEVAL_H_ */
