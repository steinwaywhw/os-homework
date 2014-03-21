#include "ring_buffer.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "zlog.h"
#include <assert.h>
#include "thread_pool.h"

zlog_category_t * category;
buffer_t buffer;

void *produce (void *args) {
	zlog_debug (category, "Producing %x by %u", args, pthread_self ());
	buffer_entry_t *entry = (buffer_entry_t *)malloc(sizeof(buffer_entry_t));
	//entry->global_seq_id = (int)args;
	buffer.produce (&buffer, entry);
	free (entry);
}

void *consume (void *args) {
	//zlog_debug (category, "Consuming %x by %u", args, pthread_self ());
	int count = 0;
	for (count = 0; count < 100; count++) {
		buffer_entry_t* entry = buffer.consume (&buffer);
		//zlog_debug (category, "Consumed %d", entry->global_seq_id);
		if (entry != 0)
			free (entry);
	}
}

int main () {

	int state = zlog_init("../zlog.conf");
	assert (state == 0);
    category = zlog_get_category("default");
	assert (category != NULL);
    zlog_debug (category, "Start!");


	buffer_config_t config = {
		.buffer_size = 10,
		.consume_threshold = 9,
		.produce_threshold = 1
	};
	init_ring_buffer (&buffer, &config);

	int i;
	int pid;

	thread_pool_t pool;
	init_thread_pool (&pool, 100);

	for (i = 0; i < 50; i++) {
		pool.submit (&pool, produce, (void *)i);
	}

	for (i = 0; i < 1; i++) {
		pool.submit (&pool, consume, (void *)i);
	}


	sleep (10);

	destroy_ring_buffer (&buffer);
	destroy_thread_pool (&pool);

	zlog_fini ();
}