// Copyright (C) 2002 by  Red Hat, Incorporated. All rights reserved.
// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <errno.h>
#include <sys/types.h>

/* The newlib implementation of these functions assumes that sizeof(char) == 1. */
char * envz_entry (const char *envz, size_t envz_len, const char *name);
char * envz_get (const char *envz, size_t envz_len, const char *name);
error_t envz_add (char **envz, size_t *envz_len, const char *name, const char *value);
error_t envz_merge (char **envz, size_t *envz_len, const char *envz2, size_t envz2_len, int override);
void envz_remove(char **envz, size_t *envz_len, const char *name);
void envz_strip (char **envz, size_t *envz_len);
