#ifndef _BUFFER_H
#define _BUFFER_H

#include "bool.h"
#include "config.h"
#include "response.h"
#include "simple_sync.h"

typedef struct {
	response_t response;
} buffer_entry_t;

typedef struct buffer_t {
	buffer_entry_t *entries;
	simple_mutex_t mutex;
	simple_cond_t cond_not_full;
	simple_cond_t cond_not_empty;

	int size;
	int count;
	int consumer_index;
	int producer_index;

	int consume_threshold;
	int produce_threshold;

	int (*produce) (struct buffer_t *buffer, buffer_entry_t *buffer_entry);
	buffer_entry_t* (*consume) (struct buffer_t *buffer);
	buffer_entry_t* (*timed_consume) (struct buffer_t *buffer, int seconds);

	boolean shutdown;
} buffer_t;

typedef struct {
	int buffer_size;
	int consume_threshold;
	int produce_threshold;
} buffer_config_t;


int init_ring_buffer (buffer_t *buffer, buffer_config_t *config);
int destroy_ring_buffer (buffer_t *buffer);

#endif


