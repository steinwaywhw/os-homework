#include "response.h"
#include <stdio.h>
#include <assert.h>
#include "bool.h"
#include <string.h>

int inline encode_res (response_t *response, char *buffer) {
	sprintf (buffer, "%u:%d:%d:", response->worker_id, response->request.client_sd, response->len);
	encode_req (&response->request, buffer + strlen (buffer));
}
int inline decode_res (response_t *response, char *data, int len) {
	assert (false);
}
