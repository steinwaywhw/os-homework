#ifndef _RES_H
#define _RES_H

#include "request.h"
#include "config.h"

typedef struct {
	unsigned int worker_id;
	unsigned int len;
	request_t request;
	void *data;
} response_t;


int encode_res (response_t *response, char *buffer);
int decode_res (response_t *response, char *data, int len);

#endif