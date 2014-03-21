#ifndef _CLIENT_H
#define _CLIENT_H

#include "socket_manager.h"
#include "bool.h"
#include <pthread.h>


struct socket_manager_t;

typedef struct client_t {
	int client_id;
	int priority;
	
	struct socket_manager_t *socket;
	pthread_t loop;

	int (*run) (struct client_t *client);

	int (*start) (struct client_t *client, char *name, boolean repeat);
	int (*stop) (struct client_t *client, char *name);
	int (*seek) (struct client_t *client, char *name, int frame);

	void *main_window;

} client_t;


int init_client (client_t *client, int priority);
int destroy_client (client_t *client);

#endif