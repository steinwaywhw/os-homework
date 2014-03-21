#include "thread_pool.h"
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include "zlog.h"

zlog_category_t *category;


void *dummy (void *args) {
	zlog_debug (category, "PID %d, TID %u, ARG %d", (int)getpid (), (int)pthread_self (), (int)args);

}


int main () {
    int state = zlog_init("../zlog.conf");
	assert (state == 0);
    category = zlog_get_category("default");
	assert (category != NULL);
    zlog_debug (category, "Start!");



	thread_pool_t pool;


	init_thread_pool (&pool, 20);
	sleep(3);
	int i;
	for (i = 0; i < 10000; i++)	{
		pool.submit (&pool, dummy, (void *)i);
	}

	sleep (3);
	destroy_thread_pool (&pool);

	zlog_fini();
}