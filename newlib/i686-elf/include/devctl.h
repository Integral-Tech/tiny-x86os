// Copyright (c) 2016,2019 Joel Sherrill <joel@rtems.org>.
// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _POSIX_DEVCTL_h_
#define _POSIX_DEVCTL_h_

/*
 * Nothing in this file should be visible unless _POSIX_26_C_SOURCE is
 * defined.
 */
#ifdef _POSIX_26_C_SOURCE

#include <sys/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__rtems__)
/*
 * The FACE Technical Standard, Edition 3.0 and later require the
 * definition of the subcommand SOCKCLOSE in <devctl.h>.
 *
 * Reference: https://www.opengroup.org/face
 *
 * Using 'D' should avoid the letters used by other users of <sys/ioccom.h>
 */
#include <sys/ioccom.h>

#define SOCKCLOSE    _IO('D', 1)    /* socket close */
#endif

/*
 * The posix_devctl() method is defined by POSIX 1003.26-2003. Aside
 * from the single method, it adds the following requirements:
 *
 *   + define _POSIX_26_VERSION to 200312L
 *   + add _SC_POSIX_26_VERSION in <unistd.h>. Return _POSIX_26_VERSION
 *   + application must define _POSIX_26_C_SOURCE to use posix_devctl().
 *   + posix_devctl() is prototyped in <devctl.h>
 */
int posix_devctl(
  int              fd,
  int              dcmd,
  void *__restrict dev_data_ptr,
  size_t           nbyte,
  int *__restrict  dev_info_ptr
);

#ifdef __cplusplus
}
#endif

#endif /* _POSIX_26_C_SOURCE */
#endif /*_POSIX_DEVCTL_h_ */
