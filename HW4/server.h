#ifndef _SERVER_H
#define _SERVER_H

#include "thread_pool_cluster.h"
#include "ring_buffer.h"
#include "file_loader.h"
#include "socket_manager.h"
#include <assert.h>
#include "config.h"

struct socket_manager_t;

typedef struct server_t {
	thread_pool_cluster_t cluster;
	buffer_t buffer;
	file_loader_t loader;
	struct socket_manager_t *socket;
	pthread_t loop;

	boolean shutdown;

	int (*run) (struct server_t *server);
} server_t;


int init_server (server_t *server);
int destroy_server (server_t *server);


#endif
