
#ifndef _SIMPLE_SYNC_H
#define _SIMPLE_SYNC_H

#include <linux/futex.h>
#include <sys/time.h>


typedef int simple_mutex_t;
typedef struct simple_cond_t {
	simple_mutex_t *mutex;
	int seq;
	int pad;
} simple_cond_t;

int simple_mutex_lock (simple_mutex_t *mutex);
int simple_mutex_unlock(simple_mutex_t *mutex);
int simple_cond_signal (simple_cond_t *cond);
int simple_cond_broadcast(simple_cond_t *cond);
int simple_cond_wait(simple_cond_t *cond, simple_mutex_t *mutex);
int simple_cond_timedwait(simple_cond_t *cond, simple_mutex_t *mutex, struct timespec *ts);


int simple_mutex_init (simple_mutex_t *mutex, void *null);
int simple_cond_init (simple_cond_t *cond, void *null);
int simple_cond_destroy (simple_cond_t *cond);
int simple_mutex_destroy (simple_mutex_t *mutex);
#endif