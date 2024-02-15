// Copyright (c) 2017  SiFive Inc. All rights reserved.
// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _FENV_H
#define _FENV_H

#include <sys/fenv.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Exception */
int feclearexcept(int excepts);
int fegetexceptflag(fexcept_t *flagp, int excepts);
int feraiseexcept(int excepts);
int fesetexceptflag(const fexcept_t *flagp, int excepts);
int fetestexcept(int excepts);

/* Rounding mode */
int fegetround(void);
int fesetround(int rounding_mode);

/* Float environment */
int fegetenv(fenv_t *envp);
int feholdexcept(fenv_t *envp);
int fesetenv(const fenv_t *envp);
int feupdateenv(const fenv_t *envp);

#ifdef __cplusplus
}
#endif

#endif
