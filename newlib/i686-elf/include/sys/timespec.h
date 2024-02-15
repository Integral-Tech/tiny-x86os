// Copyright (c) 1982, 1986, 1993
// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _SYS_TIMESPEC_H_
#define _SYS_TIMESPEC_H_

#include <sys/cdefs.h>
#include <sys/_timespec.h>

#if __BSD_VISIBLE
#define	TIMEVAL_TO_TIMESPEC(tv, ts)					\
	do {								\
		(ts)->tv_sec = (tv)->tv_sec;				\
		(ts)->tv_nsec = (tv)->tv_usec * 1000;			\
	} while (0)
#define	TIMESPEC_TO_TIMEVAL(tv, ts)					\
	do {								\
		(tv)->tv_sec = (ts)->tv_sec;				\
		(tv)->tv_usec = (ts)->tv_nsec / 1000;			\
	} while (0)

#endif /* __BSD_VISIBLE */

/*
 * Structure defined by POSIX.1b to be like a itimerval, but with
 * timespecs. Used in the timer_*() system calls.
 */
struct itimerspec {
	struct timespec  it_interval;
	struct timespec  it_value;
};

#endif /* _SYS_TIMESPEC_H_ */
