#include "client.h"
#include "request.h"
#include "response.h"
#include <pthread.h>
#include "zlog.h"
#include <assert.h>
#include <malloc.h>
#include "config.h"
#include <string.h>

//extern void _mainwindow_draw (void *main_window, void *data, int len);
//extern void mainwindow_wrong


// functions for socket manager
int _client_handle (socket_manager_t *manager, int sd);
int _write_request (int sd, request_t *request);
int _read_response (int sd, response_t *response);

// functions for client
int _client_run (client_t *client);
void* _do_client_run (void *client);

// functions for client
int _start_movie (client_t *client, char *name, boolean repeat);
int _stop_movie (client_t *client, char *name);
int _seek_movie (client_t *client, char *name, int frame);
int _show_movie (void *movie, int len);

int _show_movie (void *movie, int len) {
	assert (movie != NULL);
	assert (len > 0);

    zlog_category_t *category = zlog_get_category("client");

	char buffer[128];
	memset (buffer, 0, 128);
	memcpy (buffer, movie + 3, 38);

	zlog_info (category, "%s", buffer);
}

int _read_response (int sd, response_t *response) {
	assert (response != NULL);
	memset (response, 0, sizeof (response_t));

	int status = recv (sd, (void *)response, sizeof (response_t), MSG_WAITALL);
	assert (status != -1);

	zlog_category_t *category = zlog_get_category ("client");

	// server closed
	if (status == 0)
		return 0;

	assert (status == sizeof (response_t));

	// read movie
	if (response->len > 0) {

		// malloc here, free at _client_handle
		void *movie = malloc (response->len);
		assert (movie != NULL);

		status = recv (sd, movie, response->len, MSG_WAITALL);
		response->data = movie;
	}
	
	return sizeof (response_t);
}


int _start_movie (struct client_t *client, char *name, boolean repeat) {
	assert (client != NULL);
	assert (name != NULL);

	zlog_category_t *category = zlog_get_category ("client");
	zlog_debug (category, "Request: start movie %s", name);

	request_t request;

	request.client_id = client->client_id;
	request.priority = client->priority;
	strcpy (request.request, REQ_START);
	strcpy (request.movie, name);
	request.repeat = repeat;
	request.frame_number = 1;

	_write_request (client->socket->main_sd, &request);
}

int _stop_movie (struct client_t *client, char *name) {
	assert (client != NULL);
	assert (name != NULL);

	zlog_category_t *category = zlog_get_category ("client");
	zlog_debug (category, "Request: stop movie");

	request_t request;

	request.client_id = client->client_id;
	request.priority = client->priority;
	strcpy (request.request, REQ_STOP);
	strcpy (request.movie, name);
	request.frame_number = 1;

	_write_request (client->socket->main_sd, &request);
}

int _seek_movie (struct client_t *client, char *name, int frame) {
	assert (client != NULL);
	assert (name != NULL);

	zlog_category_t *category = zlog_get_category ("client");
	zlog_debug (category, "Request: seek movie %s", name);

	request_t request;

	request.client_id = client->client_id;
	request.priority = client->priority;
	strcpy (request.request, REQ_SEEK);
	strcpy (request.movie, name);
	request.repeat = false;
	request.frame_number = frame;

	_write_request (client->socket->main_sd, &request);
}


void* _do_client_run (void *client) {
	assert (client != NULL);
	client_t *c = (client_t *)client;

	c->socket->run (c->socket);

	pthread_exit (NULL);

}

int _client_run (client_t *client) {
	assert (client != NULL);

	zlog_category_t *category = zlog_get_category ("client");
	zlog_info (category, "Starting client loop.");

	// create thread for loop
	int status = pthread_create (&client->loop, NULL, _do_client_run, (void *)client);
	assert (status == 0);

	return 0;
}

int _write_request (int sd, request_t *request) {
	assert (request != NULL);
	assert (sd > 0);

	// get log
    zlog_category_t *category = zlog_get_category("client");
	char tmp[256];
	encode_req (request, tmp);
	zlog_info (category, "REQUEST SENT: %s", tmp);

	// do real write, blocked
	int status = send (sd, request, sizeof (request_t), 0);
	assert (status == sizeof (request_t));
}


int _client_handle (socket_manager_t *manager, int sd) {
	assert (manager != NULL);
	assert (sd > 0);

	// get log
    zlog_category_t *category = zlog_get_category("client");
    assert (category != NULL);

    // get client
	client_t *client = manager->client;

	// get response, blocked
	response_t response;
	int status = _read_response (sd, &response);

	// server closed
	if (status == 0) {
		//close (manager->main_sd);
		return -1;
	}

	// echo
	char buffer[128];
	encode_res (&response, buffer);
	zlog_info (category, "RESPONSE RECEIVED: %s", buffer);

	// deal with not exist
	if (strcmp (MSG_NOT_EXIST, response.data) == 0) {
		//mainwindow_wrong (manager->client->main_window);
		zlog_info (category, "Movie not found!");
		return 0;
	}

	_show_movie (response.data, response.len);

	//assert (manager->client->draw != NULL);
	//manager->client->draw (response.data, response.len);

	usleep (40000);
	mainwindow_draw (manager->client->main_window, response.data, response.len);
	// free it
	free (response.data);
}

int init_client (client_t *client, int priority) {
	assert (client != NULL);
	assert (priority >= 1);

	zlog_category_t *category = zlog_get_category ("client");
	zlog_info (category, "Initializing client.");

	zlog_debug (category, "Creating socket descriptor.");

	// malloc here, free at destroy
	client->socket = (socket_manager_t *)malloc (sizeof (socket_manager_t));
	assert (client->socket != NULL);

	int status = init_socket_client (client->socket, SERVER_PORT, "127.0.0.1");
	
	// server not up
	if (status < 0)
		return -1;

	// hook
	client->client_id = pthread_self ();
	client->priority = priority;
	client->socket->client = client;
	client->socket->handle = _client_handle;
	client->run = _client_run;
	client->start = _start_movie;
	client->seek = _seek_movie;
	client->stop = _stop_movie;

	return 0;
}

int destroy_client (client_t *client) {
	assert (client != NULL);

	zlog_category_t *category = zlog_get_category ("client");
	zlog_info (category, "Destroying client.");

	// destroy loop thread
	zlog_debug (category, "Destroying client loop.");
	int status = pthread_cancel (client->loop);
	status = pthread_join (client->loop, NULL);
	assert (status == 0);

	zlog_debug (category, "Destroying client socket manager.");
	destroy_socket_manager (client->socket);
	free (client->socket);
	
	return 0;
}