#include "thread_pool.h"

#define MAXPOOL 128
typedef struct thread_pool_cluster_t {
	thread_pool_t pools[MAXPOOL];
	boolean masks[MAXPOOL];

	int pool_size;
	int extend_threshold;
	int cycle_of_seconds;

	pthread_mutex_t mutex;
	boolean shutdown;

	int (*submit) (struct thread_pool_cluster_t *cluster, void *(*workload)(void *), void *args);
} thread_pool_cluster_t;

typedef struct {
	int pool_size;
	int extend_threshold;
	int cycle_of_seconds;
} thread_pool_cluster_config_t;

int init_thread_pool_cluster (thread_pool_cluster_t *cluster, thread_pool_cluster_config_t *config);
int destroy_thread_pool_cluster (thread_pool_cluster_t *cluster);