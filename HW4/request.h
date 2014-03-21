#ifndef _REQ_H
#define _REQ_H

#include "config.h"
#include <stdio.h>
#include <assert.h>


#define REQ_START 	"start_movie"
#define REQ_STOP	"stop_movie"
#define REQ_SEEK	"seek_movie"

#include "bool.h"

typedef struct {
	unsigned int client_id;
	unsigned int client_sd;
	unsigned int priority;
	unsigned char request[REQUEST_COMMAND_SIZE];
	boolean repeat;
	unsigned int frame_number;
	unsigned char movie[REQUEST_MOVIE_NAME_SIZE];
} request_t;


int encode_req (request_t *request, char *buffer);		
int decode_req (request_t *request, char *data, int len);	

#endif