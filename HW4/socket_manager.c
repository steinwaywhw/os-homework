
#include "socket_manager.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <netdb.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>
#include "bool.h"
#include "config.h"
#include "zlog.h"


int _run_client (socket_manager_t *manager);
int _run_server (socket_manager_t *manager);

int init_socket_client (socket_manager_t *manager, int port, char *host) {
	assert (manager != NULL);
	assert (port > 0);
	assert (host != NULL);

    manager->shutdown = false;

	zlog_category_t *category = zlog_get_category ("socket");
	zlog_info (category, "Initializing client socket manager with %s:%d.", host, port);

	// create sd
	zlog_debug (category, "Creating socket descriptor.");
	manager->port = port;
	strcpy (manager->host, host);
	manager->main_sd = socket (AF_INET, SOCK_STREAM, 0);
	
	assert (manager->main_sd > 0);

	// resolve host address
	zlog_debug (category, "Resolving host name.");
	struct hostent *hent;
	hent = gethostbyname (host);
	assert (hent != NULL);

	// prepare for connect
	memset ((void *)&manager->name, 0, sizeof (struct sockaddr_in));
	manager->name.sin_family = AF_INET;
	manager->name.sin_port = htons (port);
    bcopy (hent->h_addr, &manager->name.sin_addr, hent->h_length);

    // connect
    zlog_debug (category, "Connecting to server.");
    int status = connect (manager->main_sd, (struct sockaddr *)&manager->name, sizeof(struct sockaddr_in));
	// server not up
	if (status < 0)
		return -1;

    // change buffer
    int n = SOCK_RECV_BUFFER;
    setsockopt (manager->main_sd, SOL_SOCKET, SO_RCVBUF, &n, sizeof(n));

    //status = fcntl (manager->main_sd, F_GETFL, 0);
	//status = fcntl (manager->main_sd, F_SETFL, ~O_NONBLOCK);

    // hook
    manager->run = _run_client;

	return 0;
}

int init_socket_server (socket_manager_t *manager, int port) {
	assert (manager != NULL);
	assert (port > 0);

	zlog_category_t *category = zlog_get_category ("socket");
	zlog_info (category, "Initializing server socket manager at %d.", port);

    manager->shutdown = false;

	// create sd
	zlog_debug (category, "Creating socket descriptor.");
	manager->port = port;
	manager->main_sd = socket (AF_INET, SOCK_STREAM, 0);
	assert (manager->main_sd > 0);

	// prepare for bind
	memset ((void *)&manager->name, 0, sizeof (struct sockaddr_in));
	manager->name.sin_family = AF_INET;
	manager->name.sin_port = htons (port);
	manager->name.sin_addr.s_addr = htonl (INADDR_ANY);

	// bind
	zlog_debug (category, "Binding");
	int status = bind (manager->main_sd, (struct sockaddr *)&manager->name, sizeof (struct sockaddr_in));
	assert (status == 0);

	// listen
	zlog_debug (category, "Listening at %d.", port);
	status = listen (manager->main_sd, MAX_CONN);
	assert (status == 0);

    // sent buffer
    int n = SOCK_SEND_BUFFER;
    setsockopt (manager->main_sd, SOL_SOCKET, SO_SNDBUF, &n, sizeof(n));

    // hook
    manager->run = _run_server;

	return 0;
}




int _run_client (socket_manager_t *manager) {

	assert (manager != NULL);
	assert (manager->client != NULL);

	zlog_category_t *category = zlog_get_category ("socket");
	zlog_info (category, "Running client loop.");

	while (manager->shutdown == false) {
		manager->handle (manager, manager->main_sd);
	}

	zlog_debug (category, "Destroying client socket manager.");	
}

int _run_server (socket_manager_t *manager) {

	assert (manager != NULL);
	assert (manager->server != NULL);

	zlog_category_t *category = zlog_get_category ("socket");
	zlog_info (category, "Running server loop.");

	while (manager->shutdown == false) {

		// accept
		int size = sizeof (struct sockaddr_in);
		int sd = accept (manager->main_sd, (struct sockaddr *)&manager->name, &size);
		assert (sd >= 0);
		zlog_info (category, "New connection accepted at descriptor %d.", sd);

		// set sent buffer
	    int n = SOCK_SEND_BUFFER;
	    setsockopt (sd, SOL_SOCKET, SO_SNDBUF, &n, sizeof(n));

		manager->handle (manager, sd);
	}

	zlog_debug (category, "Destroying server socket manager.");	
}

int destroy_socket_manager (socket_manager_t *manager) {
	assert (manager != NULL);

	zlog_category_t *category = zlog_get_category ("socket");
	zlog_info (category, "Destroying socket manager.");

	manager->shutdown = true;
	close (manager->main_sd);

	return 0;
}