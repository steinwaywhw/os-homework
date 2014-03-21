
#include "ring_buffer.h"
#include <pthread.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include "bool.h"
#include "zlog.h"
#include <sys/time.h>
#include <errno.h>

int _produce (buffer_t *buffer, buffer_entry_t *buffer_entry);
buffer_entry_t* _consume (buffer_t *buffer);
buffer_entry_t* _timed_consume (buffer_t *buffer, int seconds);
buffer_entry_t* _allocate_entry ();

int init_ring_buffer (buffer_t *buffer, buffer_config_t *config) {
	assert (buffer != NULL);
	assert (config != NULL);
	assert (config->buffer_size > 0);
	assert (config->consume_threshold <= config->buffer_size && config->consume_threshold >= 1);
	assert (config->produce_threshold < config->buffer_size && config->produce_threshold >= 0);


	zlog_category_t *category = zlog_get_category ("ring_buffer");
	assert (category != NULL);

	zlog_debug (category, "Initializing ring buffer with size %d.", config->buffer_size);
	
	// malloc
	zlog_debug (category, "Allocating buffer memory.");
	buffer->entries = (buffer_entry_t *)malloc(config->buffer_size * sizeof (buffer_entry_t));
	assert (buffer->entries != NULL);

	// reset buffer
	memset (buffer->entries, sizeof (buffer->entries), 0);

	// setup constants
	buffer->size = config->buffer_size;
	buffer->count = 0;
	buffer->consumer_index = 0;
	buffer->producer_index = 0;
	buffer->shutdown = false;
	buffer->consume_threshold = config->consume_threshold;
	buffer->produce_threshold = config->produce_threshold;

	// setup sync facilities
	zlog_debug (category, "Initializing mutex.");
	int status;
	status = pthread_mutex_init (&buffer->mutex, NULL);
	assert (status == 0);
	status = pthread_cond_init (&buffer->cond_not_full, NULL);
	assert (status == 0);
	status = pthread_cond_init (&buffer->cond_not_empty, NULL);
	assert (status == 0);

	// hooking methods
	buffer->produce = _produce;
	buffer->consume = _consume;
	buffer->timed_consume = _timed_consume;
	return 0;
}

int destroy_ring_buffer (buffer_t *buffer) {
	assert (buffer != NULL);

	zlog_category_t *category = zlog_get_category ("ring_buffer");
	assert (category != NULL);

	buffer->shutdown = true;

	zlog_debug (category, "Destroying ring buffer.");

	


	int status;

	// NOTES: Destroy any user before destroy ring buffer!
	//status = pthread_mutex_lock (&buffer->mutex);
	//assert (status == 0);

	zlog_debug (category, "Destroying mutex.");

	pthread_cond_broadcast (&buffer->cond_not_full);
	status = pthread_cond_destroy (&buffer->cond_not_full);
	assert (status == 0);

	pthread_cond_broadcast (&buffer->cond_not_empty);
	status = pthread_cond_destroy (&buffer->cond_not_empty);
	assert (status == 0);

	status = pthread_mutex_unlock (&buffer->mutex);
	assert (status == 0);
	status = pthread_mutex_destroy (&buffer->mutex);
	while (status != 0) {
		status = pthread_mutex_destroy (&buffer->mutex);
	}
	assert (status == 0);
	

	zlog_debug (category, "Reclaiming memory.");
	assert (&buffer->entries != NULL);
	free (buffer->entries);

	return 0;
}

int _produce (buffer_t *buffer, buffer_entry_t *buffer_entry) {
	assert (buffer != NULL);
	assert (buffer_entry != NULL);

	zlog_category_t *category = zlog_get_category ("ring_buffer");
	assert (category != NULL);
	zlog_debug (category, "Producer of TID %u", pthread_self ());


	// if shutdown
	if (buffer->shutdown == true) {
		zlog_debug (category, "Shutting down producer.");
		return 0;
	}

	// try to lock buffer
	zlog_debug (category, "Waiting for lock.");
	int status = pthread_mutex_lock (&buffer->mutex);
	assert (status == 0);
	zlog_debug (category, "Locked.");


	// wait for not full
	while (buffer->count >= buffer->consume_threshold) {  
		zlog_debug (category, "Buffer threshold not reached. Waiting for consumers.");
	    status = pthread_cond_wait (&buffer->cond_not_full, &buffer->mutex);  
	    assert (status == 0);

	    // if shutdown
	    if (buffer->shutdown == true) {
	    	status = pthread_mutex_unlock (&buffer->mutex);
	    	assert (status == 0);
	    	return 0;
	    }
	}  	

	assert (buffer->count < buffer->consume_threshold);
	//assert (buffer->count < buffer->size);

	// copy entry to buffer
	zlog_debug (category, "Writing to buffer.");
	memcpy (&(buffer->entries)[buffer->producer_index], buffer_entry, sizeof (buffer_entry_t));
	
	// update count
	zlog_debug (category, "Updating status.");
	buffer->count++;
	assert (buffer->count <= buffer->size);

	buffer->producer_index = (buffer->producer_index + 1) % buffer->size;
	zlog_debug (category, "Count %d, PI %d, CI %d", buffer->count, buffer->producer_index, buffer->consumer_index);

	// signal
	if (buffer->count == buffer->consume_threshold) {
		zlog_debug (category, "Signalling non-empty condition.");
	    status = pthread_cond_broadcast (&buffer->cond_not_empty);  
	    assert (status == 0);
	}  
	

	// unlock
	zlog_debug (category, "Finish writing. Unlock.");
	status = pthread_mutex_unlock (&buffer->mutex);  
	assert (status == 0);



	return 0;
}

buffer_entry_t* _allocate_entry () {
	buffer_entry_t* ret = (buffer_entry_t *)malloc(sizeof (buffer_entry_t));
	assert (ret != NULL);
	return ret;
}

buffer_entry_t* _consume (buffer_t *buffer) {

	assert (buffer != NULL);
	zlog_category_t *category = zlog_get_category ("ring_buffer");
	assert (category != NULL);

	zlog_debug (category, "Consumer of TID %u", pthread_self ());



	// if shutdown
	if (buffer->shutdown == true) {
		zlog_debug (category, "Shutting down consumer.");
		return 0;
	}

	// try to lock
	zlog_debug (category, "Waiting for lock.");
	int status = pthread_mutex_lock (&buffer->mutex);      
	assert (status == 0);
	zlog_debug (category, "Locked.");

	// wait for non-empty
	while (buffer->count <= buffer->produce_threshold) {  
		zlog_debug (category, "Buffer threshold not reached. Waiting for producers.");
	    status = pthread_cond_wait (&buffer->cond_not_empty, &buffer->mutex);  
	    assert (status == 0);

	    // if shutdown
	    if (buffer->shutdown == true) {
	    	status = pthread_mutex_unlock (&buffer->mutex);
	    	assert (status == 0);
	    	return 0;
	    }
	} 

	assert (buffer->count > buffer->produce_threshold);
	//assert (buffer->count > 0);

	// allocate entry
	zlog_debug (category, "Reading from buffer.");
	buffer_entry_t *entry = _allocate_entry ();

	memcpy (entry, &(buffer->entries)[buffer->consumer_index], sizeof (buffer_entry_t));
		

	// update
	zlog_debug (category, "Updating status.");
	buffer->count--;
	assert (buffer->count >= 0);
	buffer->consumer_index = (buffer->consumer_index + 1) % buffer->size;
	zlog_debug (category, "Count %d, PI %d, CI %d", buffer->count, buffer->producer_index, buffer->consumer_index);

	// non full
	if (buffer->count == buffer->produce_threshold) {  
		zlog_debug (category, "Signalling non-full condition.");
	    status = pthread_cond_broadcast (&buffer->cond_not_full);  
	    assert (status == 0);
	}  

	// unlock
	zlog_debug (category, "Finish reading. Unlock.");
	status = pthread_mutex_unlock (&buffer->mutex);  
	assert (status == 0);

	

	return entry;
}






buffer_entry_t* _timed_consume (buffer_t *buffer, int seconds) {

	assert (buffer != NULL);
	zlog_category_t *category = zlog_get_category ("ring_buffer");
	assert (category != NULL);

	zlog_debug (category, "Consumer of TID %u", pthread_self ());

	int status;
	struct timespec   ts;
	struct timeval    tp;

 	status = gettimeofday(&tp, NULL);
 	assert (status == 0);

    // convert from timeval to timespec
    ts.tv_sec  = tp.tv_sec;
    ts.tv_nsec = tp.tv_usec * 1000;
    ts.tv_sec += seconds;

	// if shutdown
	if (buffer->shutdown == true) {
		zlog_debug (category, "Shutting down consumer.");
		return 0;
	}

	// try to lock
	zlog_debug (category, "Waiting for lock.");
	status = pthread_mutex_lock (&buffer->mutex);      
	assert (status == 0);
	zlog_debug (category, "Locked.");

	// wait for non-empty
	while (buffer->count <= buffer->produce_threshold) {  
		zlog_debug (category, "Buffer threshold not reached. Waiting for producers.");
	    status = pthread_cond_timedwait (&buffer->cond_not_empty, &buffer->mutex, &ts);  

		if (status == ETIMEDOUT) {
			zlog_debug (category, "Wait timed out!");
			status = pthread_mutex_unlock(&buffer->mutex);
			assert (status == 0);

			return NULL;
		}

	    // if shutdown
	    if (buffer->shutdown == true) {
	    	status = pthread_mutex_unlock (&buffer->mutex);
	    	assert (status == 0);
	    	return 0;
	    }
	} 

	assert (buffer->count > buffer->produce_threshold);
	//assert (buffer->count > 0);

	// allocate entry
	zlog_debug (category, "Reading from buffer.");
	buffer_entry_t *entry = _allocate_entry ();

	memcpy (entry, &(buffer->entries)[buffer->consumer_index], sizeof (buffer_entry_t));
		

	// update
	zlog_debug (category, "Updating status.");
	buffer->count--;
	assert (buffer->count >= 0);
	buffer->consumer_index = (buffer->consumer_index + 1) % buffer->size;
	zlog_debug (category, "Count %d, PI %d, CI %d", buffer->count, buffer->producer_index, buffer->consumer_index);

	// non full
	if (buffer->count == buffer->produce_threshold) {  
		zlog_debug (category, "Signalling non-full condition.");
	    status = pthread_cond_broadcast (&buffer->cond_not_full);  
	    assert (status == 0);
	}  

	// unlock
	zlog_debug (category, "Finish reading. Unlock.");
	status = pthread_mutex_unlock (&buffer->mutex);  
	assert (status == 0);

	

	return entry;
}