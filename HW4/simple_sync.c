#include "simple_sync.h"
#include <assert.h>
#include "bool.h"
#include <xmmintrin.h>
#include <linux/futex.h>
#include <limits.h>
#include <errno.h>
#include <sys/time.h>


static long sys_futex(void *addr1, int op, int val1, struct timespec *timeout, void *addr2, int val3) {
	return syscall(240, addr1, op, val1, timeout, addr2, val3);
}

int cmpxchg (int *p, int oldval, int newval) {
 	return __sync_val_compare_and_swap (p, oldval, newval);
}

int xchg_32 (int *p, int newval) {
	return __sync_lock_test_and_set (p, newval);
}

int atomic_add (int *p, int val) {
	__sync_add_and_fetch (p, val);
}

int simple_mutex_init (simple_mutex_t *mutex, void *null) {
	assert (mutex != NULL);
	*mutex = 0;
	return 0;
}

int simple_mutex_destroy (simple_mutex_t *mutex) {
	assert (mutex != NULL);
	return 0;
}

int simple_mutex_lock (simple_mutex_t *mutex) {
	int i, c;
	
	/* Spin and try to take lock */
	for (i = 0; i < 100; i++)
	{
		c = cmpxchg(mutex, 0, 1);
		if (!c) 
			return 0;
		
		_mm_pause();
	}

	/* The lock is now contended */
	if (c == 1) 
		c = xchg_32(mutex, 2);

	while (c)
	{
		/* Wait in the kernel */
		sys_futex(mutex, FUTEX_WAIT_PRIVATE, 2, NULL, NULL, 0);
		c = xchg_32(mutex, 2);
	}
	
	return 0;
}

int simple_mutex_unlock(simple_mutex_t *mutex) {
	int i;
	
	/* Unlock, and if not contended then exit. */
	if (*mutex == 2) {
		*mutex = 0;
	} else if (xchg_32(mutex, 0) == 1) 
		return 0;

	/* Spin and hope someone takes the lock */
	for (i = 0; i < 200; i++) {
		if (*mutex) {
			/* Need to set to state 2 because there may be waiters */
			if (cmpxchg(mutex, 1, 2)) 
				return 0;
		}
		_mm_pause();
	}
	
	/* We need to wake someone up */
	sys_futex(mutex, FUTEX_WAKE_PRIVATE, 1, NULL, NULL, 0);
	
	return 0;
}

int simple_cond_init (simple_cond_t *cond, void *null) {
	assert (cond != NULL);
	cond->mutex = NULL;
	cond->seq = 0;

	return 0;
}

int simple_cond_destroy (simple_cond_t *cond) {
	assert (cond != NULL);
	return 0;
}

int simple_cond_signal (simple_cond_t *cond) {
	/* We are waking someone up */
	atomic_add(&cond->seq, 1);
	
	/* Wake up a thread */
	sys_futex(&cond->seq, FUTEX_WAKE_PRIVATE, 1, NULL, NULL, 0);
	
	return 0;
}

int simple_cond_broadcast(simple_cond_t *cond)
{
	simple_mutex_t *mutex = cond->mutex;
	
	/* No mutex means that there are no waiters */
	if (!mutex) return 0;
	
	/* We are waking everyone up */
	atomic_add(&cond->seq, 1);
	
	/* Wake one thread, and requeue the rest on the mutex */
	sys_futex(&cond->seq, FUTEX_REQUEUE_PRIVATE, 1, (void *)INT_MAX, mutex, 0);
	
	return 0;
}

int simple_cond_wait(simple_cond_t *cond, simple_mutex_t *mutex)
{
	int seq = cond->seq;

	if (cond->mutex != mutex)
	{
		if (cond->mutex) 
			return -1;
		
		/* Atomically set mutex inside cv */
		cmpxchg(&cond->mutex, NULL, mutex);
		if (cond->mutex != mutex) 
			return -1;
	}
	
	simple_mutex_unlock(mutex);
	
	sys_futex(&cond->seq, FUTEX_WAIT_PRIVATE, seq, NULL, NULL, 0);
	
	while (xchg_32(mutex, 2))
	{
		sys_futex(mutex, FUTEX_WAIT_PRIVATE, 2, NULL, NULL, 0);
	}
		
	return 0;
}

int simple_cond_timedwait(simple_cond_t *cond, simple_mutex_t *mutex, struct timespec *ts)
{
	int seq = cond->seq;

	if (cond->mutex != mutex)
	{
		if (cond->mutex) 
			return -1;
		
		/* Atomically set mutex inside cv */
		cmpxchg(&cond->mutex, NULL, mutex);
		if (cond->mutex != mutex) 
			return -1;
	}
	
	simple_mutex_unlock(mutex);
	
	int status = sys_futex(&cond->seq, FUTEX_WAIT_PRIVATE, seq, ts, NULL, 0);
	
	while (xchg_32(mutex, 2))
	{
		sys_futex(mutex, FUTEX_WAIT_PRIVATE, 2, NULL, NULL, 0);
	}
		
	return status;
}
