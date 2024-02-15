// Copyright (c) 2011 Ed Schouten <ed@FreeBSD.org>
// SPDX-FileCopyrightText: 2024 Integral <integral@member.fsf.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _THREADS_H_
#define	_THREADS_H_

#include <machine/_threads.h>
#include <time.h>

typedef void (*tss_dtor_t)(void *);
typedef int (*thrd_start_t)(void *);

enum {
	mtx_plain = 0x1,
	mtx_recursive = 0x2,
	mtx_timed = 0x4
};

enum {
	thrd_busy = 1,
	thrd_error = 2,
	thrd_nomem = 3,
	thrd_success = 4,
	thrd_timedout = 5
};

#if !defined(__cplusplus) || __cplusplus < 201103L
#define	thread_local		_Thread_local
#endif

__BEGIN_DECLS
void	call_once(once_flag *, void (*)(void));
int	cnd_broadcast(cnd_t *);
void	cnd_destroy(cnd_t *);
int	cnd_init(cnd_t *);
int	cnd_signal(cnd_t *);
int	cnd_timedwait(cnd_t *__restrict, mtx_t *__restrict __mtx,
    const struct timespec *__restrict)
    __requires_exclusive(*__mtx);
int	cnd_wait(cnd_t *, mtx_t *__mtx)
    __requires_exclusive(*__mtx);
void	mtx_destroy(mtx_t *__mtx)
    __requires_unlocked(*__mtx);
int	mtx_init(mtx_t *__mtx, int)
    __requires_unlocked(*__mtx);
int	mtx_lock(mtx_t *__mtx)
    __locks_exclusive(*__mtx);
int	mtx_timedlock(mtx_t *__restrict __mtx,
    const struct timespec *__restrict)
    __trylocks_exclusive(thrd_success, *__mtx);
int	mtx_trylock(mtx_t *__mtx)
    __trylocks_exclusive(thrd_success, *__mtx);
int	mtx_unlock(mtx_t *__mtx)
    __unlocks(*__mtx);
int	thrd_create(thrd_t *, thrd_start_t, void *);
thrd_t	thrd_current(void);
int	thrd_detach(thrd_t);
int	thrd_equal(thrd_t, thrd_t);
_Noreturn void
	thrd_exit(int);
int	thrd_join(thrd_t, int *);
int	thrd_sleep(const struct timespec *, struct timespec *);
void	thrd_yield(void);
int	tss_create(tss_t *, tss_dtor_t);
void	tss_delete(tss_t);
void *	tss_get(tss_t);
int	tss_set(tss_t, void *);
__END_DECLS

#endif /* !_THREADS_H_ */
