// Copyright (c) 1990, 1993
// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _NDBM_H_
#define	_NDBM_H_

/* #include <db.h> */

/*
 * The above header-file is directly included in `newlib/libc/search/ndbm.c`
 * as `db.h` is present in form of `db_local.h`, inside `newlib/libc/search`
 * directory and not in `newlib/libc/include`.
 * Necessary data-types are mentioned in form of forward-declarations
 */
   
/* Map dbm interface onto db(3). */
#define DBM_RDONLY	O_RDONLY

/* Flags to dbm_store(). */
#define DBM_INSERT      0
#define DBM_REPLACE     1

/*
 * The db(3) support for ndbm always appends this suffix to the
 * file name to avoid overwriting the user's original database.
 */
#define	DBM_SUFFIX	".db"

typedef struct {
	void *dptr;
	int dsize;	/* XXX Should be size_t according to 1003.1-2008. */
} datum;

struct __db;      /* Forward-declaration */
typedef struct __db DB;   /* Forward-declaration */
typedef DB DBM;
#define	dbm_pagfno(a)	DBM_PAGFNO_NOT_AVAILABLE

__BEGIN_DECLS
int	 dbm_clearerr(DBM *);
void	 dbm_close(DBM *);
int	 dbm_delete(DBM *, datum);
int	 dbm_error(DBM *);
datum	 dbm_fetch(DBM *, datum);
datum	 dbm_firstkey(DBM *);
datum	 dbm_nextkey(DBM *);
DBM	*dbm_open(const char *, int, mode_t);
int	 dbm_store(DBM *, datum, datum, int);
#if __BSD_VISIBLE
int	 dbm_dirfno(DBM *);
#endif
__END_DECLS

#endif /* !_NDBM_H_ */
