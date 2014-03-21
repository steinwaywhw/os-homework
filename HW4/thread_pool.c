#include "thread_pool.h"
#include <pthread.h>
#include <assert.h>
#include <string.h>
#include <semaphore.h>
#include "zlog.h"
#include <malloc.h>
#include "bool.h"




void *_thread_pool_worker (void *args);
task_t *_dequeue (task_queue_t *queue);
void _enqueue (task_queue_t *queue, void *(*fn) (void *), void *args);
int _submit (thread_pool_t *pool, void *(*workload)(void *), void *args);


task_t *_dequeue (task_queue_t *queue) {
	assert (queue != NULL);
	assert (queue->head != NULL);

	task_t *ret = queue->head;

	if (queue->head == queue->tail) {
		queue->head = NULL;
		queue->tail = NULL;
	} else {
		queue->head = queue->head->next;
	}

	queue->count--;
	assert (queue->count >= 0);
	return ret;
}


void _enqueue (task_queue_t *queue, void *(*fn) (void *), void *args) {
	assert (queue != NULL);
	assert (fn != NULL);

	zlog_category_t *category = zlog_get_category ("thread_pool");


	// malloc here, free at _thread_pool_worker
	task_t *task = (task_t *)malloc(sizeof (task_t));
	assert (task != NULL);

	task->task = fn;
	task->args = args;
	task->next = NULL;

	if (queue->tail == NULL && queue->head == NULL) {
		queue->head = task;
		queue->tail = task;
	} else {
		queue->tail->next = task;
		queue->tail = task;
	}

	int status = sem_post (&queue->sem);
	assert (status == 0);
	queue->count++;
}

void *_thread_pool_worker (void *args) {

	assert (args != 0);
	thread_pool_t *pool = (thread_pool_t *)args;

	zlog_category_t *category = zlog_get_category ("thread_pool");

	while (pool->shutdown == false) {
		// Wait for new job
		zlog_debug (category, "Thread %u is waiting for new job.", pthread_self ());
		int state = sem_wait (&pool->tasks.sem);
		assert (state == 0);

		// Shutdown
		if (pool->shutdown == true)
			return NULL;

		// Wait for queue operation
		zlog_debug (category, "Thread %u is peeking new job.", pthread_self ());
		int status = pthread_mutex_lock (&pool->mutex);
		assert (status == 0);
		pool->working_count++;
		task_t *task= _dequeue (&pool->tasks);
		status = pthread_mutex_unlock (&pool->mutex);
		assert (status == 0);

		// Do the job
		zlog_debug (category, "Thread %u is running the job.", pthread_self ());
		task->task (task->args);

		//free here, malloc at _enqueue
		free (task);

		status = pthread_mutex_lock (&pool->mutex);
		assert (status == 0);
		pool->working_count--;
		status = pthread_mutex_unlock (&pool->mutex);
		assert (status == 0);
	}

	pthread_exit (NULL);
}

int _submit (thread_pool_t *pool, void *(*workload)(void *), void *args) {
	assert (pool != NULL);
	assert (workload != NULL);

	zlog_category_t *category = zlog_get_category ("thread_pool");

	zlog_debug (category, "Submitting new job.");

	int status = pthread_mutex_lock (&pool->mutex);
	assert (status == 0);

	_enqueue (&pool->tasks, workload, args);
	status = pthread_mutex_unlock (&pool->mutex);
	assert (status == 0);

	return 0;
}


int init_thread_pool (thread_pool_t *pool, int pool_size) {
	assert (pool != NULL);
	assert (pool_size > 0);

	zlog_category_t *category = zlog_get_category ("thread_pool");
	zlog_debug (category, "Initiating thread pool of size %d.", pool_size);

	// init pool size and shutdown states
	pool->size = pool_size;
	pool->shutdown = false;

	// init task queue
	pool->tasks.head = 0;
	pool->tasks.tail = 0;
	pool->tasks.count = 0;

	// init mutex
	int status;
	status = pthread_mutex_init (&pool->mutex, NULL);
	assert (status == 0);

	// init sem
	status = sem_init (&pool->tasks.sem, 0, 0);
	assert (status == 0);

	// init all poolled threads
	pool->threads = (pthread_t *)malloc(pool_size * sizeof (pthread_t));
	assert (pool->threads != 0);
	pool->working_count = 0;

	int i;
	for (i = 0; i < pool_size; i++) {
		zlog_debug (category, "Creating thread %d", i);
		status = pthread_create (&(pool->threads[i]), NULL, _thread_pool_worker, (void *)pool);
		assert (status == 0);
	}

	pool->submit = _submit;
}

int destroy_thread_pool (thread_pool_t *pool) {
	assert (pool != NULL);
	zlog_category_t *category = zlog_get_category ("thread_pool");

	zlog_debug (category,"Destroying thread pool.");

	// Shutdown workers
	zlog_debug (category, "Shutting down all workers.");
	pool->shutdown = true;

	int i;
	int status;
	for (i = 0; i < pool->size; i++) {
		status = sem_post (&pool->tasks.sem);
		assert (status == 0);
	}

	for (i = 0; i < pool->size; i++) {
		status = pthread_cancel (pool->threads[i]);
		//assert (status == 0);
		status = pthread_join (pool->threads[i], NULL);
		assert (status == 0);
	}

	free (pool->threads);


	// Destroy semaphore
	zlog_debug (category, "Destroying semaphore.");
	status = sem_destroy (&pool->tasks.sem);
	assert (status == 0);

	// Destroy task queue
	zlog_debug (category, "Destroying task queue.");

	task_t *curr = pool->tasks.head;
	while (curr != NULL) {
		task_t *next = curr->next;
		free (curr);
		curr = next;
	}

	// Destroy mutex
	status = pthread_mutex_destroy (&pool->mutex);
	assert (status == 0);

	return 0;
}

