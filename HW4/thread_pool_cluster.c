#include "thread_pool_cluster.h"
#include <assert.h>
#include "zlog.h"

void *_moniter (void *args);
int _cluster_submit (thread_pool_cluster_t *cluster, void *(*workload)(void *), void *args);

int init_thread_pool_cluster (thread_pool_cluster_t *cluster, thread_pool_cluster_config_t *config) {
	assert (cluster != NULL);
	assert (config != NULL);

	zlog_category_t *category = zlog_get_category ("thread_cluster");
	zlog_info (category, "Initializing thread pool cluster.");
	zlog_info (category, "Pool size %d, Extension threshold %d.", config->pool_size, config->extend_threshold);

	cluster->pool_size = config->pool_size;
	cluster->extend_threshold = config->extend_threshold;
	cluster->cycle_of_seconds = config->cycle_of_seconds;
	cluster->shutdown = false;

	init_thread_pool (&cluster->pools[0], cluster->pool_size);

	int i;
	for (i = 0; i < MAXPOOL; i++) 
		cluster->masks[i] = false;
	cluster->masks[0] = true;

	cluster->submit = _cluster_submit;

	// init mutex
	pthread_mutex_init (&cluster->mutex, NULL);

	// submit the monitor task
	zlog_debug (category, "Submitting monitor thread.");
	cluster->pools[0].submit (&cluster->pools[0], _moniter, cluster);
}

int destroy_thread_pool_cluster (thread_pool_cluster_t *cluster) {
	zlog_category_t *category = zlog_get_category ("thread_cluster");
	zlog_info (category, "Destroying thread pool cluster.");
	cluster->shutdown = true;
	int i;
	for (i = 0; i < MAXPOOL; i++) {
		if (cluster->masks[i] == true)
			destroy_thread_pool (&cluster->pools[i]);
	}

	pthread_mutex_destroy (&cluster->mutex);

	return 0;
}

int _cluster_submit (thread_pool_cluster_t *cluster, void *(*workload)(void *), void *args) {
	assert (workload != NULL);

	zlog_category_t *category = zlog_get_category ("thread_cluster");
	zlog_info (category, "Submitting task to cluster.");

	zlog_debug (category, "Finding proper thread pool.");
	// try to find a pool whose workload < extend_threshold
	int i;
	int found = false;
	for (i = 0; i < MAXPOOL; i++) {
		if (cluster->masks[i] == false)
			continue;

		if (cluster->pools[i].tasks.count > cluster->extend_threshold)
			continue;
		else {
			found = true;
			break;
		}
	}

	// found, submit
	if (found == true) {
		zlog_debug (category, "Found. Submit to pool %d.", i);
		cluster->pools[i].submit (&cluster->pools[i], workload, args);
		return 0;
	} 

	// else, extend
	for (i = 0; i < MAXPOOL; i++) {
		if (cluster->masks[i] == false)
			break;
	}

	// cluster full. Just submit to the first one
	if (i == MAXPOOL) {
		zlog_debug (category, "Not found, and pool full. Submit to pool 0.");
		cluster->pools[0].submit (&cluster->pools[0], workload, args);
		return 0;
	}

	// create one
	zlog_info (category, "Not found. Create and submit to the new pool %d.", i);
	int status = pthread_mutex_lock (&cluster->mutex);
	assert (status == 0);
	init_thread_pool (&cluster->pools[i], cluster->pool_size);
	cluster->masks[i] = true;
	status = pthread_mutex_unlock (&cluster->mutex);
	assert (status == 0);

	cluster->pools[i].submit (&cluster->pools[i], workload, args);
	return 0;
}

int inline _count (thread_pool_cluster_t *cluster) {
	int sum = 0, i;
	for (i = 0; i < MAXPOOL; i++)
		if (cluster->masks[i] == true)
			sum++;

	return sum;
}

void *_moniter (void *args) {
	assert (args != NULL);
	thread_pool_cluster_t *cluster = (thread_pool_cluster_t *)args;

	int i;
	int status;
	zlog_category_t *category = zlog_get_category ("thread_cluster");

	while (cluster->shutdown == false) {
		sleep (cluster->cycle_of_seconds);

		
		zlog_info (category, "Cluster monitor invoked. Count %d.", _count(cluster));

		for (i = 1; i < MAXPOOL; i++) {

			// if it is an used thread pool
			if (cluster->masks[i] == false)
				continue;

			// and no one is in the critical section
			status = pthread_mutex_trylock (&cluster->pools[i].mutex);
			if (status != 0) {
				pthread_mutex_unlock (&cluster->pools[i].mutex);
				continue;
			}

			// and there is no pending tasks
			if (cluster->pools[i].tasks.count != 0) {
				pthread_mutex_unlock (&cluster->pools[i].mutex);
				continue;
			}

			// and working count is zero
			if (cluster->pools[i].working_count != 0) {
				pthread_mutex_unlock (&cluster->pools[i].mutex);
				continue;
			}

			// then destroy it
			int status = pthread_mutex_lock (&cluster->mutex);
			assert (status == 0);
			zlog_info (category, "Destroying pool %d.", i);

			pthread_mutex_unlock (&cluster->pools[i].mutex);
			destroy_thread_pool (&cluster->pools[i]);
			cluster->masks[i] = false;
			status = pthread_mutex_unlock (&cluster->mutex);
			assert (status == 0);
		}
	}



	pthread_exit (NULL);
}
