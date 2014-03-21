#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include "bool.h"
#include <semaphore.h>

// For internal usage
typedef struct task_t {
	void *(*task) (void *args);
	void *args;
	struct task_t *next;
} task_t;

typedef struct {
	task_t *head;
	task_t *tail;
	sem_t sem;
	int count;
} task_queue_t;


// For public usage

typedef struct thread_pool_t {
	int size;
	pthread_t *threads;
	task_queue_t tasks;
	boolean shutdown;

	pthread_mutex_t mutex;
	int working_count;
	int (*submit) (struct thread_pool_t *pool, void *(*workload)(void *), void *args);
} thread_pool_t;



int init_thread_pool (thread_pool_t *pool, int pool_size);
int destroy_thread_pool (thread_pool_t *pool);

#endif