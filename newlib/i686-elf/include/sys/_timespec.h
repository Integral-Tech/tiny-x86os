// Copyright (c) 1982, 1986, 1993
// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _SYS__TIMESPEC_H_
#define	_SYS__TIMESPEC_H_

#include <sys/_types.h>

#if !defined(__time_t_defined) && !defined(_TIME_T_DECLARED)
typedef	_TIME_T_	time_t;
#define	__time_t_defined
#define	_TIME_T_DECLARED
#endif

struct timespec {
	time_t	tv_sec;		/* seconds */
	long	tv_nsec;	/* and nanoseconds */
};

#endif /* !_SYS__TIMESPEC_H_ */
