#include "alt_server.h"
#include "request.h"
#include <malloc.h>
#include <string.h>
#include "sortlist.h"
#include <time.h>
#include "movies.h"
#include <pthread.h>
#include "zlog.h"

// functions for socket manager
int _server_handle (socket_manager_t *manager, int sd);

// functions for server
int _server_run (server_t *server);
void* _do_server_run (void *server);

int _read_request (int sd, request_t *request);
int _write_response (int sd, response_t *response);

void* _server_worker (void *args);
void* _server_producer (void *args);
void* _server_consumer (void *args);

typedef struct {
	server_t *server;
	int sd;
} worker_arg_t;

typedef struct {
	server_t *server;
	unsigned int worker_id;
	request_t request;
	boolean *shutdown;
	pthread_mutex_t *mutex;
} producer_arg_t;

boolean inline _check_movie (request_t *request, server_t *server) {
    char filename[256];

    // frame number is also checked
    sprintf (filename, "%s%u.ppm", request->movie, request->frame_number);
    return server->loader.exists (&server->loader, filename);
}

int inline _load_movie (request_t *request, server_t *server, void *buffer) {

	char filename[256];
	sprintf (filename, "%s%u.ppm", request->movie, request->frame_number);

	int fd = server->loader.open (&server->loader, filename);
	return server->loader.read (&server->loader, fd, 0, MOVIE_FRAME_BUFFER_SIZE, buffer);
}

void* _do_server_run (void *server) {
	assert (server != NULL);
	server_t *s = (server_t *)server;

	s->socket->run (s->socket);

	pthread_exit (NULL);
}

int _server_run (server_t *server) {
	assert (server != NULL);

    zlog_category_t *category = zlog_get_category("server");
	zlog_info (category, "Starting server loop.");

	// create thread for loop
	int status = pthread_create (&server->loop, NULL, _do_server_run, (void *)server);
	assert (status == 0);

	return 0;
}


void* _server_producer (void *args) {

	assert (args != NULL);

	// get data and free it
	producer_arg_t *p = (producer_arg_t *)args;
	server_t *server = p->server;
	unsigned int worker_id = p->worker_id;
	boolean *shutdown = p->shutdown;
	boolean loop = p->request.repeat;

	pthread_mutex_t *mutex = p->mutex;

	// get log
    zlog_category_t *category = zlog_get_category("server");
    zlog_debug (category, "Producer: producing.");

	// create entry
	buffer_entry_t entry;
	memset (&entry, 0, sizeof (buffer_entry_t));
	memcpy (&entry.response.request, &p->request, sizeof (request_t));
	entry.response.worker_id = worker_id;

	// free it
	free (args);

	// used for determine if it is truly finished
	pthread_mutex_lock (mutex);

	// loop to produce movie until end of movie or stop signalled
	while (*shutdown == false) {

		int exist = _check_movie (&entry.response.request, server);

		if (exist == false && loop == true) {
			entry.response.request.frame_number = 1;
			continue;
		} else if (exist == false && loop == false) {
			break;
		}

		// malloc here, free at _server_consumer, not _write
		void *movie = malloc (MOVIE_FRAME_BUFFER_SIZE);

		// load movie
		int len = _load_movie (&entry.response.request, server, movie);

		// setup entry
		entry.response.len = len;
		entry.response.data = movie;

		// copy into ring buffer
		server->buffer.produce (&server->buffer, &entry);
		
		entry.response.request.frame_number++;

		// wait a little bit
		//usleep (10);
	}

	pthread_mutex_unlock (mutex);
}

int _write_response (int sd, response_t *response) {
	assert (sd > 0);
	assert (response != NULL);

	// get log
    zlog_category_t *category = zlog_get_category("server");
    assert (category != NULL);

	// do real write, block
	int status = send (sd, response, sizeof (response_t), 0);
	//assert (status == sizeof (response_t));

	// client closed
	if (status < sizeof (response_t))
		return -1;

	// write movie
	status = send (sd, response->data, response->len, 0);
	//assert (status == response->len);
	
	// client closed
	if (status < response->len)
		return -1;


	return sizeof (request_t);
}




int _read_request (int sd, request_t *request) {
	assert (request != NULL);
	memset (request, 0, sizeof (request_t));

	zlog_category_t *category = zlog_get_category ("server");

	// block and receive
	int n = recv (sd, (void *)request, sizeof (request_t), MSG_WAITALL);
	//assert (n != -1);

	// client closed or error
	if (n <= 0)
		return 0;

	assert (n == sizeof (request_t));
	return n;
}

int _server_handle (socket_manager_t *manager, int sd) {

	assert (manager != NULL);
	assert (manager->server != NULL);

	// get log
    zlog_category_t *category = zlog_get_category("server");
    assert (category != NULL);

    // get server
	server_t *server = manager->server;

	// malloc here, free at _server_worker
	worker_arg_t *arg = (worker_arg_t *)malloc (sizeof (worker_arg_t));
	assert (arg != NULL);

	arg->server = server;
	arg->sd = sd;

	// submit
	server->cluster.submit (&server->cluster, _server_worker, (void *)arg);
}

void* _server_worker (void *args) {

	assert (args != NULL);

	// get data and free it
	worker_arg_t *p = (worker_arg_t *)args;
	server_t *server = p->server;
	int sd = p->sd;

	free (args);

	// get log
    zlog_category_t *category = zlog_get_category("server");
    zlog_debug (category, "Worker: serve descripter %d.", sd);

    // producer shutdown signal
    boolean producer_shutdown = false;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

    // worker id
    unsigned int worker_id = pthread_self ();

	// loop
	while (server->shutdown == false) {

		// block and read
		request_t request;
		int status = _read_request (sd, &request);

		// update sd
		request.client_sd = sd;

		// client closed
		if (status == 0) {
			zlog_info (category, "Client at %d closed.", sd);
			producer_shutdown = true;
			pthread_mutex_lock (&mutex);
			pthread_mutex_unlock (&mutex);
			close (sd);
			break;
		}

		assert (status == sizeof (request_t));

		// debug
		char tmp[256];
		encode_req (&request, tmp);
		zlog_info (category, "REQUEST RECEIVED: %s", tmp);

		// deal with NOT_EXIST
		if (_check_movie (&request, server) == false) {
					

			response_t response;
			memcpy (&response.request, &request, sizeof (request_t));

			response.worker_id = pthread_self ();

			response.data = malloc ((strlen (MSG_NOT_EXIST) + 1) * sizeof (char));
			strcpy (response.data, MSG_NOT_EXIST);

			response.len = strlen (MSG_NOT_EXIST) + 1;

			// block and write
			_write_response (sd, (void *)&response);


			// free
			free (response.data);

			break;
		}

		// malloc here, free at _producer
		producer_arg_t *arg = (producer_arg_t *)malloc (sizeof (producer_arg_t));
		assert (arg != NULL);
		memset (arg, 0, sizeof (producer_arg_t));

		// setup
		arg->server = server;
		arg->worker_id = worker_id;
		memcpy (&arg->request, &request, sizeof (request_t));
		arg->shutdown = &producer_shutdown;
		arg->mutex = &mutex;

		// decode command
		if (strcmp (request.request, REQ_START) == 0) {
			// start a movie

			// shutdown previous producer
			producer_shutdown = true;
			pthread_mutex_lock (&mutex);
			pthread_mutex_unlock (&mutex);

			producer_shutdown = false;
			
			// start from 1
			arg->request.frame_number = 1;
			producer_shutdown = false;

			// submit
			server->cluster.submit (&server->cluster, _server_producer, (void *)arg);

		} else if (strcmp (request.request, REQ_SEEK) == 0) {
			// shutdown previous producer
			producer_shutdown = true;
			pthread_mutex_lock (&mutex);
			pthread_mutex_unlock (&mutex);

			producer_shutdown = false;
			arg->request.frame_number = request.frame_number;

			// submit
			server->cluster.submit (&server->cluster, _server_producer, (void *)arg);

		} else if (strcmp (request.request, REQ_STOP) == 0) {
			producer_shutdown = true;
		} else {
			// command not supported
			assert (false);
		}
	}

	pthread_mutex_destroy (&mutex);
}

void* _server_consumer (void *args) {

	assert (args != NULL);

	// get log
    zlog_category_t *category = zlog_get_category("server");

    // get some params
	server_t *server = (server_t *)args;
	buffer_t *buffer = &server->buffer;

	sortlist_t *sortlist = NULL;
	buffer_entry_t *entry = NULL;

	// entries to be sent
	int len = 0;

	// wait threshold. if it is too long
	// send those entries even if they are less than a limit
	time_t then = time (NULL);

	while (server->shutdown == false) {

		// consume an entry from buffer
		entry = buffer->consume (buffer);

		if (entry != NULL) {
			// store it in a temp list
			sortlist = sortlist_insert (sortlist, entry);
			len++;
		}

		// update time
		time_t now = time (NULL);

		// send them when: 1 enough entries, 2 enough time
		if (server->shutdown == false 
			&& ((len == DISPATCH_ENTRY_THRESHOLD) 
				|| (len > 0 && ((now - then) >= DISPATCH_TIME_THRESHOLD)))) {

			// write
			while (sortlist != NULL) {

				sortlist = sortlist_remove (sortlist, &entry);

				char tmp[256];
				encode_res (&entry->response, tmp);
				zlog_info (category, "DISPATCH: %s", tmp);

				zlog_debug (category, "Consumer: dispatch a response to %d.", entry->response.request.client_sd);

				// block and write
				int status = _write_response (entry->response.request.client_sd, (void *)&entry->response);
				
				if (status < 0) {
					close (entry->response.request.client_sd);
				}

				// safe to free it
				free (entry->response.data);
				free (entry);

				len--;
			}

			assert (sortlist == NULL);

			// update
			then = time (NULL);
		}
	}

	return NULL;
}

int init_server (server_t *server) {
	assert (server != NULL);

	server->shutdown = false;

    zlog_category_t *category = zlog_get_category("server");
	zlog_info (category, "Initializing server.");

	// init thread cluster
	thread_pool_cluster_config_t config_cluster = {
		.pool_size = 10,
		.extend_threshold = 5,
		.cycle_of_seconds = 1
	};
	init_thread_pool_cluster (&server->cluster, &config_cluster);

	// init file loader 
	// filenames are defined in movies.h
	init_file_loader (&server->loader, 100, "../images/", filenames);

	// init ring buffer
	buffer_config_t config_buffer = {
		.buffer_size = 50,
		.consume_threshold = 1,
		.produce_threshold = 0
	};
	init_ring_buffer (&server->buffer, &config_buffer);

	// malloc here, free at destroy_server
	server->socket = (socket_manager_t *)malloc (sizeof (socket_manager_t));
	assert (server->socket != NULL);

	// init socket
	init_socket_server (server->socket, SERVER_PORT);

	// hook
	server->socket->handle = _server_handle;
	server->socket->server = server;
	server->run = _server_run;

	// submit consumer
	server->cluster.submit (&server->cluster, _server_consumer, (void *)server);

	return 0;
}

int destroy_server (server_t *server) {
	assert (server != NULL);

	server->shutdown = true;

    zlog_category_t *category = zlog_get_category("server");
	zlog_info (category, "Destroying server.");

	// destroy loop thread
	zlog_debug (category, "Destroying server loop.");
	int status = pthread_cancel (server->loop);
	status = pthread_join (server->loop, NULL);
	assert (status == 0);

	zlog_debug (category, "Destroying file loader.");
	destroy_file_loader (&server->loader);


	zlog_debug (category, "Destroying server socket manager.");
	destroy_socket_manager (server->socket);
	free (server->socket);

	zlog_debug (category, "Destroying thread cluster.");
	destroy_thread_pool_cluster (&server->cluster);


	zlog_debug (category, "Destroying ring buffer.");
	destroy_ring_buffer (&server->buffer);

	return 0;
}