// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _SYS_UTIME_H
#define _SYS_UTIME_H

/* This is a dummy <sys/utime.h> file, not customized for any
   particular system.  If there is a utime.h in libc/sys/SYSDIR/sys,
   it will override this one.  */

#ifdef __cplusplus
extern "C" {
#endif

struct utimbuf 
{
  time_t actime;
  time_t modtime; 
};

#ifdef __cplusplus
};
#endif

#endif /* _SYS_UTIME_H */
