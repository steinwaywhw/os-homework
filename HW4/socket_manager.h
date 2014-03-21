#ifndef _SOCKET_H
#define _SOCKET_H

#include "bool.h"
#include "server.h"
#include "client.h"
#include <netinet/in.h>
#include "config.h"




struct server_t;
struct client_t;

typedef struct socket_manager_t {
	int port;
	char host[MAX_HOSTNAME];
	int main_sd;

	boolean shutdown;

	struct sockaddr_in name;
	struct server_t *server;
	struct client_t *client;

	int (*handle) (struct socket_manager_t *manager, int sd);
	int (*run) (struct socket_manager_t *manager);
} socket_manager_t;

int init_socket_server (socket_manager_t *manager, int port);
int init_socket_client (socket_manager_t *manager, int port, char *host);

int destroy_socket_manager (socket_manager_t *manager);

#endif