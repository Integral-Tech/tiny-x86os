// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

/* envlock.h -- header file for env routines.  */

#ifndef _INCLUDE_ENVLOCK_H_
#define _INCLUDE_ENVLOCK_H_

#include <_ansi.h>
#include <sys/reent.h>

#define ENV_LOCK __env_lock(reent_ptr)
#define ENV_UNLOCK __env_unlock(reent_ptr)

void __env_lock (struct _reent *reent);
void __env_unlock (struct _reent *reent);

#endif /* _INCLUDE_ENVLOCK_H_ */
